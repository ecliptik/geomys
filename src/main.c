/*
 * main.c - Geomys: A Gopher browser for classic Macintosh
 * Targeting System 6.0.8 / Macintosh Plus with MacTCP 2.1
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Memory.h>
#include <SegLoad.h>
#include <OSUtils.h>
#include <ToolUtils.h>
#include <Resources.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "menus.h"
#include "dialogs.h"
#include "macutil.h"
#include "gopher.h"
#include "gopher_types.h"
#include "browser.h"
#include "content.h"
#include "history.h"
#include "settings.h"
#include "favorites.h"
#ifdef GEOMYS_OFFSCREEN
#include "offscreen.h"
#endif
#ifdef GEOMYS_CACHE
#include "cache.h"
#endif
#include "color.h"
#include "theme.h"
#ifdef GEOMYS_DOWNLOAD
#include <Files.h>
#include "savefile.h"
#include "imgparse.h"
#endif

/* Globals */
Boolean g_running = true;
Boolean g_suspended = false;
GeomysPrefs g_prefs;

/* Saved system key repeat settings (restored on quit) */
static short saved_key_thresh;
static short saved_key_rep_thresh;

/* Notification Manager — alert user when page loads in background */
static NMRec g_nm_rec;
static Boolean g_nm_posted = false;

/* Download progress dialog */
#ifdef GEOMYS_DOWNLOAD
static DialogPtr g_dl_dialog = 0L;
static void dl_progress_open(void);
static void dl_progress_update(long bytes);
static void dl_progress_close(void);
#endif

/* g_window, g_app_state, g_gopher are macros in main.h
 * pointing to active_session fields.
 * g_pending_scroll is also per-session: */
#define g_pending_scroll (active_session->pending_scroll)

/* Forward declarations */
static void init_toolbox(void);
static void init_apple_events(void);
static void create_session_window(BrowserSession *s);
static void init_session(BrowserSession *s);
static void main_event_loop(void);
static void handle_mouse_down(EventRecord *event);
static void handle_key_down(EventRecord *event);
static void handle_update(EventRecord *event);
static void handle_activate(EventRecord *event);
/* do_navigate_url declared in main.h */
static void handle_nav_button(short btn_id);
static void handle_action_button(void);
static void update_nav_buttons(void);
void navigate_history_entry(const HistoryEntry *e, short direction);
static void handle_page_loaded(void);

/*
 * Apple Events handlers for System 7 compatibility.
 */
static pascal OSErr
ae_open_app(const AppleEvent *evt, AppleEvent *reply, long refcon)
{
	(void)evt; (void)reply; (void)refcon;
	return noErr;
}

static pascal OSErr
ae_quit_app(const AppleEvent *evt, AppleEvent *reply, long refcon)
{
	(void)evt; (void)reply; (void)refcon;
	g_running = false;
	return noErr;
}

static pascal OSErr
ae_open_doc(const AppleEvent *evt, AppleEvent *reply, long refcon)
{
	(void)evt; (void)reply; (void)refcon;
	return errAEEventNotHandled;
}

static pascal OSErr
ae_print_doc(const AppleEvent *evt, AppleEvent *reply, long refcon)
{
	(void)evt; (void)reply; (void)refcon;
	return errAEEventNotHandled;
}

static void
init_apple_events(void)
{
	long resp;

	if (Gestalt(gestaltAppleEventsAttr, &resp) == noErr &&
	    (resp & 1)) {
		AEInstallEventHandler(kCoreEventClass,
		    kAEOpenApplication,
		    NewAEEventHandlerUPP(ae_open_app), 0L, false);
		AEInstallEventHandler(kCoreEventClass,
		    kAEQuitApplication,
		    NewAEEventHandlerUPP(ae_quit_app), 0L, false);
		AEInstallEventHandler(kCoreEventClass,
		    kAEOpenDocuments,
		    NewAEEventHandlerUPP(ae_open_doc), 0L, false);
		AEInstallEventHandler(kCoreEventClass,
		    kAEPrintDocuments,
		    NewAEEventHandlerUPP(ae_print_doc), 0L, false);
	}
}

int
main(void)
{
	init_toolbox();
	init_apple_events();

	/* Load preferences before menus so checkmarks are correct */
	prefs_load(&g_prefs);

#ifdef GEOMYS_COLOR
	color_detect();
#endif
#ifdef GEOMYS_THEMES
	theme_init(g_prefs.theme_id);
#endif

	init_menus();

	/* Create first session with window, chrome, and content */
	{
		BrowserSession *s = session_new();
		if (!s) {
			SysBeep(10);
			ExitToShell();
		}
		create_session_window(s);
#if GEOMYS_MAX_WINDOWS > 1
		active_session = s;
#endif
		init_session(s);
	}

	update_menus();

#ifdef GEOMYS_FAVORITES
	/* Initialize favorites menu */
	favorites_init_menu(&g_prefs);
#endif

#ifdef GEOMYS_CACHE
	cache_init();
#endif

	/* Pre-load MacTCP driver so first connection is fast */
	SetCursor(*GetCursor(watchCursor));
	browser_set_status("Loading MacTCP\311");
	{
		GrafPtr save;
		GetPort(&save);
		SetPort(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	}
	conn_init_tcp();
	InitCursor();

	/* Navigate to home page (or show blank if empty) */
	if (g_prefs.home_url[0])
		do_navigate_url(g_prefs.home_url);
	else
		browser_set_status("Ready");

	/* Save initial state so first window's session has correct
	 * status/URL — matches do_new_window() pattern */
	session_save_state(active_session);

	main_event_loop();
	return 0;
}

static void
init_toolbox(void)
{
	SetApplLimit(LMGetApplLimit() - (1024 * 8));
	MaxApplZone();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	MoreMasters();
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();

	/* Save system key repeat and set fast repeat for keyboard nav.
	 * Default PRAM is often ~18 ticks (300ms) between repeats.
	 * We set 2 ticks (33ms) ≈ 30 cps for responsive scrolling. */
	saved_key_thresh = LMGetKeyThresh();
	saved_key_rep_thresh = LMGetKeyRepThresh();
	LMSetKeyThresh(12);		/* 200ms initial delay */
	LMSetKeyRepThresh(2);		/* 33ms repeat = ~30 cps */
}

/*
 * create_session_window - Create the Mac window for a session.
 * Uses cascading position for multi-window builds.
 */
static short g_cascade_count = 0;

static void
create_session_window(BrowserSession *s)
{
	Rect bounds;
#if GEOMYS_MAX_WINDOWS > 1
	short offset = (g_cascade_count % 4) * CASCADE_OFFSET;
	short w = SCREEN_WIDTH - 4;
	short h = SCREEN_HEIGHT - 44;

	g_cascade_count++;
	SetRect(&bounds, 2 + offset, 42 + offset,
	    2 + offset + w, 42 + offset + h);
#else
	SetRect(&bounds, 2, 42, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2);
#endif

	/* procID 8 = zoomDocProc: document window with zoom box + grow box */
	/* Use NewCWindow on color systems for CGrafPort, NewWindow on B&W */
	if (g_has_color_qd) {
		s->window = NewCWindow(0L, &bounds, "\pGeomys", true,
		    8, (WindowPtr)-1L, true, 0L);
	} else {
		s->window = NewWindow(0L, &bounds, "\pGeomys", true,
		    8, (WindowPtr)-1L, true, 0L);
	}
}

/*
 * init_session - Initialize browser chrome, content, gopher, and history
 * for a newly created session. Assumes window already exists.
 */
static void
init_session(BrowserSession *s)
{
	GrafPtr save;

	GetPort(&save);
	SetPort(s->window);
	browser_init(s->window);
	content_init(s->window);
#ifdef GEOMYS_OFFSCREEN
	offscreen_init(s->window);
#endif
	SetPort(save);

	/* Initialize Gopher engine and history */
	gopher_init(&s->gopher);
	s->gopher.conn.dns_server = ip2long(g_prefs.dns_server);
	content_set_page(&s->gopher);
	history_init();

	/* Save module statics into this session */
	browser_save_state(s);
	content_save_state(s);
	history_save_state(s);
}

/*
 * do_new_window - Create a new browser window (Cmd-N).
 * Allocates a session, creates window, initializes, and
 * navigates to home page.
 */
void
do_new_window(void)
{
	BrowserSession *s;

#if GEOMYS_MAX_WINDOWS > 1 
	/* Save current session state before creating new one */
	if (active_session)
		session_save_state(active_session);
#endif

	s = session_new();
	if (!s) {
		if (session_count() >= GEOMYS_MAX_WINDOWS)
			ParamText(
			    "\pMaximum number of windows "
			    "reached.",
			    "\p", "\p", "\p");
		else
			ParamText(
			    "\pNot enough memory. Try "
			    "closing other windows or "
			    "applications.",
			    "\p", "\p", "\p");
		StopAlert(128, 0L);
		return;
	}

	create_session_window(s);
	if (!s->window) {
		session_destroy(s);
		ParamText(
		    "\pNot enough memory. Try closing "
		    "other windows or applications.",
		    "\p", "\p", "\p");
		StopAlert(128, 0L);
		return;
	}

#if GEOMYS_MAX_WINDOWS > 1
	active_session = s;
#endif

	init_session(s);

	/* Navigate to home page or show blank */
	if (g_prefs.home_url[0])
		do_navigate_url(g_prefs.home_url);
	else
		browser_set_status("Ready");

	/* Save initial state so session switch restores it */
	session_save_state(s);
}

/*
 * handle_page_loaded - Handle completion of a Gopher page load.
 * Updates title, address bar, status bar, caches page,
 * and redraws. Must be called with correct session active.
 */
static void
handle_page_loaded(void)
{
	char uri[300];
	GrafPtr save;

#ifdef GEOMYS_DOWNLOAD
	/* Download completion — close file, show result, restore UI */
	if (g_gopher.page_type == PAGE_DOWNLOAD) {
		char fname[64];
		short flen;

		g_app_state = APP_STATE_IDLE;
		InitCursor();
		dl_progress_close();

		/* Extract C string filename for status */
		flen = g_gopher.dl_filename[0];
		if (flen > 63) flen = 63;
		memcpy(fname, g_gopher.dl_filename + 1, flen);
		fname[flen] = '\0';

		if (g_gopher.dl_refnum) {
			FSClose(g_gopher.dl_refnum);
			FlushVol(0L, g_gopher.dl_vrefnum);
		}

		if (g_gopher.dl_error) {
			/* Write error — delete partial file */
			if (g_gopher.dl_filename[0])
				FSDelete(g_gopher.dl_filename,
				    g_gopher.dl_vrefnum);
			show_error_alert(
			    "Error writing file. "
			    "The disk may be full.");
		} else if (g_gopher.conn.timed_out &&
		    g_gopher.dl_written == 0) {
			show_error_alert(
			    "The download failed. "
			    "The connection timed out "
			    "before any data was "
			    "received.");
		} else {
			snprintf(uri, sizeof(uri),
			    "Downloaded %s \xD0 %ld bytes",
			    fname, g_gopher.dl_written);
			browser_set_status(uri);
		}

		/* Reset download state */
		g_gopher.dl_refnum = 0;
		g_gopher.dl_written = 0;
		g_gopher.dl_error = false;
		g_gopher.dl_vrefnum = 0;
		g_gopher.dl_filename[0] = 0;

		/* Restore previous page and redraw. The old page
		 * data (items/text) was preserved during download
		 * so the directory listing is still intact. */
		g_gopher.page_type = g_gopher.dl_prev_page;

		/* Restore title bar from history */
		{
			const HistoryEntry *he;

			he = history_current();
			if (he && he->title[0])
				set_wtitlef(g_window, "%s",
				    he->title);
			else
				set_wtitlef(g_window, "%s",
				    g_gopher.cur_host);
		}

		/* Redraw entire window and update button state */
		update_nav_buttons();
		GetPort(&save);
		SetPort(g_window);
		content_mark_all_dirty();
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw(g_window);
		SetPort(save);

		return;
	}

	/* Image download completion — close file, show image
	 * info, restore UI. Header bytes were already written
	 * to disk in gopher_process_data when sniffing completed. */
	if (g_gopher.page_type == PAGE_IMAGE) {
		g_app_state = APP_STATE_IDLE;
		InitCursor();
		dl_progress_close();

		/* If image was smaller than sniff buffer, the
		 * header bytes weren't written yet — write them
		 * now before closing the file. */
		if (g_gopher.dl_refnum &&
		    !g_gopher.img_sniffed &&
		    g_gopher.img_header_len > 0) {
			long hc = g_gopher.img_header_len;
			OSErr he = FSWrite(g_gopher.dl_refnum,
			    &hc, g_gopher.img_header);
			if (he != noErr)
				g_gopher.dl_error = true;
			g_gopher.dl_written += hc;
		}

		if (g_gopher.dl_refnum) {
			FSClose(g_gopher.dl_refnum);
			FlushVol(0L, g_gopher.dl_vrefnum);
		}

		if (g_gopher.dl_error) {
			if (g_gopher.dl_filename[0])
				FSDelete(g_gopher.dl_filename,
				    g_gopher.dl_vrefnum);
			show_error_alert(
			    "Error writing file. "
			    "The disk may be full.");
		} else if (g_gopher.conn.timed_out &&
		    g_gopher.dl_written == 0) {
			show_error_alert(
			    "The download failed. "
			    "The connection timed out "
			    "before any data was "
			    "received.");
		} else {
			/* Success — show image info */
			Str255 pmsg;
			char msg[200];
			short fmt;
			unsigned short img_w, img_h;

			fmt = img_detect_format(
			    g_gopher.img_header,
			    g_gopher.img_header_len);

			if (img_parse_dimensions(
			    g_gopher.img_header,
			    g_gopher.img_header_len,
			    fmt, &img_w, &img_h)) {
				snprintf(msg, sizeof(msg),
				    "Image saved. "
				    "Format: %s, "
				    "Size: %ld bytes, "
				    "Dimensions: %u \xD7 %u",
				    img_format_name(
				    g_gopher.img_header,
				    g_gopher.img_header_len,
				    fmt),
				    g_gopher.dl_written,
				    img_w, img_h);
			} else {
				snprintf(msg, sizeof(msg),
				    "Image saved. "
				    "Format: %s, "
				    "Size: %ld bytes",
				    img_format_name(
				    g_gopher.img_header,
				    g_gopher.img_header_len,
				    fmt),
				    g_gopher.dl_written);
			}

			c2pstr(pmsg, msg);
			ParamText(pmsg, "\p", "\p", "\p");
			NoteAlert(128, 0L);

			snprintf(uri, sizeof(uri),
			    "Image saved \xD0 %ld bytes",
			    g_gopher.dl_written);
			browser_set_status(uri);
		}

		/* Reset download + image state */
		g_gopher.dl_refnum = 0;
		g_gopher.dl_written = 0;
		g_gopher.dl_error = false;
		g_gopher.dl_vrefnum = 0;
		g_gopher.dl_filename[0] = 0;
		g_gopher.img_header_len = 0;
		g_gopher.img_sniffed = false;

		/* Restore previous page and redraw */
		g_gopher.page_type = g_gopher.dl_prev_page;

		/* Restore title bar from history */
		{
			const HistoryEntry *he;

			he = history_current();
			if (he && he->title[0])
				set_wtitlef(g_window, "%s",
				    he->title);
			else
				set_wtitlef(g_window, "%s",
				    g_gopher.cur_host);
		}

		/* Redraw entire window and update button state */
		update_nav_buttons();
		GetPort(&save);
		SetPort(g_window);
		content_mark_all_dirty();
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw(g_window);
		SetPort(save);

		return;
	}
#endif

	g_app_state = APP_STATE_IDLE;
	InitCursor();

	{
		const HistoryEntry *he;

		he = history_current();
		if (he && he->title[0])
			set_wtitlef(g_window, "%s",
			    he->title);
		else
			set_wtitlef(g_window, "%s",
			    g_gopher.cur_host);
	}

	/* Update address bar and status */
	gopher_build_uri(uri, sizeof(uri),
	    g_gopher.cur_host, g_gopher.cur_port,
	    g_gopher.cur_type, g_gopher.cur_selector);
	browser_set_url(uri);

	if (g_gopher.conn.timed_out) {
		browser_set_status("Connection timed out");
		g_gopher.conn.timed_out = false;
	} else if (g_gopher.cur_type == GOPHER_ERROR) {
		browser_set_status("Server Error");
	} else {
		if (g_gopher.page_type == PAGE_DIRECTORY)
			snprintf(uri, sizeof(uri),
			    "Done \xD0 %d items",
			    g_gopher.item_count);
		else
			snprintf(uri, sizeof(uri),
			    "Done \xD0 %ld bytes",
			    g_gopher.text_len);
		browser_set_status(uri);
	}

	/* Post notification if loading completed in background */
	if (g_suspended && !g_nm_posted) {
		memset(&g_nm_rec, 0, sizeof(g_nm_rec));
		g_nm_rec.qType = 8;	/* nmType */
		g_nm_rec.nmMark = 1;
		g_nm_rec.nmSound = (Handle)-1;
		g_nm_rec.nmIcon = 0L;
		g_nm_rec.nmStr = 0L;
		g_nm_rec.nmResp = 0L;	/* nil — we handle removal on resume */
		NMInstall(&g_nm_rec);
		g_nm_posted = true;
	}

	/* Cache the loaded page */
#ifdef GEOMYS_CACHE
	cache_store(active_session->id,
	    history_current_index(), &g_gopher);
#endif

	/* Update nav button states */
	update_nav_buttons();

	GetPort(&save);
	SetPort(g_window);

	/* Calculate content width for horizontal scrollbar */
	content_recalc_width(g_window);

	/* Restore deferred scroll position */
	if (g_pending_scroll >= 0) {
		content_update_scroll(g_window);
		content_set_scroll_pos(g_pending_scroll);
		g_pending_scroll = -1;
	}

	/* Force full redraw on page load completion.
	 * Incremental draws during loading may leave the shadow
	 * buffer valid with stale dirty flags, causing the final
	 * draw to skip rows if a hover event set a dirty flag
	 * between intermediate draws. */
	content_mark_all_dirty();
	content_draw(g_window);
	content_update_scroll(g_window);
	browser_draw(g_window);
	SetPort(save);

	/* Persist final state so session switch restores
	 * correct status bar, address bar, and scrollbar */
	session_save_state(active_session);
}

static void
main_event_loop(void)
{
	EventRecord event;
	long wait_ticks;

	while (g_running) {
		if (g_suspended) {
			wait_ticks = 60L;
#if GEOMYS_MAX_WINDOWS > 1 
		} else {
			short si;

			wait_ticks = 3L;
			for (si = 0; si < GEOMYS_MAX_WINDOWS; si++) {
				BrowserSession *bs = session_get(si);
				if (bs && bs->gopher.receiving) {
					wait_ticks = 1L;
					break;
				}
			}
#else
		} else if (g_app_state == APP_STATE_LOADING) {
			wait_ticks = 1L;
		} else {
			wait_ticks = 3L;
#endif
		}

		WaitNextEvent(everyEvent, &event, wait_ticks, 0L);

		switch (event.what) {
		case nullEvent:
#if GEOMYS_MAX_WINDOWS > 1
			/* Poll ALL sessions for incoming data.
			 * Background sessions: only poll connection,
			 * do NOT draw (avoids offscreen buffer
			 * conflicts). Drawing happens on activate
			 * or when the session is the active one.
			 * Throttle background to every 4th tick. */
			{
				BrowserSession *orig = active_session;
				short si;
				static unsigned short bg_tick;

				bg_tick++;
				for (si = 0; si < GEOMYS_MAX_WINDOWS;
				    si++) {
					BrowserSession *bs;

					bs = session_get(si);
					if (!bs ||
					    !bs->gopher.receiving)
						continue;

					/* Active session: batch
				 * TCP reads with draw
				 * deadline to avoid
				 * parse-draw-parse-draw */
					if (bs == orig) {
						unsigned long dd;
						short dc = 0;
						Boolean nd = false;

						dd = TickCount() + 4;
						while (g_gopher.receiving
						    && dc < 16) {
							if (gopher_idle(
							    &g_gopher))
								nd = true;
							dc++;
							if (TickCount()
							    >= dd)
								break;
						}
						if (nd) {
							GrafPtr sp;
							char pg[80];

#ifdef GEOMYS_DOWNLOAD
							if (g_gopher.page_type
							    == PAGE_DOWNLOAD ||
							    g_gopher.page_type
							    == PAGE_IMAGE) {
								if (!g_dl_dialog)
									dl_progress_open();
								dl_progress_update(
								    g_gopher.dl_written);
							} else
#endif
							{
							if (g_gopher.page_type
							    == PAGE_DIRECTORY)
								snprintf(pg,
								    sizeof(pg),
								    "Loading\311 %d items",
								    g_gopher.item_count);
							else
								snprintf(pg,
								    sizeof(pg),
								    "Loading\311 %ld bytes",
								    g_gopher.text_len);
							browser_set_status(pg);
							}

							GetPort(&sp);
							SetPort(
							    g_window);
#ifdef GEOMYS_DOWNLOAD
							if (g_gopher.page_type
							    != PAGE_DOWNLOAD &&
							    g_gopher.page_type
							    != PAGE_IMAGE) {
#endif
							content_draw(
							    g_window);
							content_update_scroll(
							    g_window);
#ifdef GEOMYS_DOWNLOAD
							}
#endif
							browser_draw_status(
							    g_window);
							SetPort(sp);
						}
						if (!g_gopher.receiving)
							handle_page_loaded();
						continue;
					}

					/* Background: throttle */
					if ((bg_tick & 3) != 0)
						continue;

					/* Poll connection only —
					 * no drawing, no state
					 * switch needed since
					 * gopher_idle operates
					 * directly on the struct */
					(void)gopher_idle(
					    &bs->gopher);
					if (!bs->gopher.receiving) {
						/* Page done: do full
						 * switch for
						 * handle_page_loaded */
						session_switch_to(bs);
						SetPort(bs->window);
						handle_page_loaded();
					}
				}

				/* Restore original active session */
				if (active_session != orig) {
					session_switch_to(orig);
					if (orig && orig->window)
						SetPort(orig->window);
				}
			}
#else
			/* Poll Gopher connection for data.
			 * Batch TCP reads with 4-tick draw
			 * deadline to prevent per-read redraws
			 * on large directories. */
			if (g_gopher.receiving) {
				unsigned long draw_deadline;
				short drain_count = 0;
				Boolean needs_draw = false;

				draw_deadline = TickCount() + 4;
				while (g_gopher.receiving
				    && drain_count < 16) {
					if (gopher_idle(&g_gopher))
						needs_draw = true;
					drain_count++;
					if (TickCount() >= draw_deadline)
						break;
				}
				if (needs_draw) {
					GrafPtr save;
					char prog[80];

#ifdef GEOMYS_DOWNLOAD
					if (g_gopher.page_type ==
					    PAGE_DOWNLOAD ||
					    g_gopher.page_type ==
					    PAGE_IMAGE) {
						if (!g_dl_dialog)
							dl_progress_open();
						dl_progress_update(
						    g_gopher.dl_written);
					} else
#endif
					{
					if (g_gopher.page_type ==
					    PAGE_DIRECTORY)
						snprintf(prog,
						    sizeof(prog),
						    "Loading\311 %d items",
						    g_gopher.item_count);
					else
						snprintf(prog,
						    sizeof(prog),
						    "Loading\311 %ld bytes",
						    g_gopher.text_len);
					browser_set_status(prog);
					}

					GetPort(&save);
					SetPort(g_window);
#ifdef GEOMYS_DOWNLOAD
					if (g_gopher.page_type !=
					    PAGE_DOWNLOAD &&
					    g_gopher.page_type !=
					    PAGE_IMAGE) {
#endif
					content_draw(g_window);
					content_update_scroll(
					    g_window);
#ifdef GEOMYS_DOWNLOAD
					}
#endif
					browser_draw_status(g_window);
					SetPort(save);
				}
				if (!g_gopher.receiving)
					handle_page_loaded();
			}
#endif

			/* Address bar cursor blink + content cursor update */
			if (!g_suspended && g_window) {
				GrafPtr save;
				Point mouse_pt;

				GetPort(&save);
				SetPort(g_window);
				browser_idle();

				/* Update cursor based on mouse position */
				GetMouse(&mouse_pt);
				if (!browser_cursor_update(g_window,
				    mouse_pt))
					content_cursor_update(g_window,
					    mouse_pt);

				SetPort(save);
			}
			break;
		case keyDown:
#ifdef GEOMYS_DOWNLOAD
			/* Cmd-. cancels download even with dialog up */
			if (g_dl_dialog &&
			    (event.modifiers & cmdKey) &&
			    (event.message & charCodeMask) == '.') {
				do_cancel_loading();
				break;
			}
#endif
			handle_key_down(&event);
			break;
		case autoKey:
			if (!(event.modifiers & cmdKey)) {
				char ak = event.message & charCodeMask;

				if (browser_get_focus() != FOCUS_ADDR_BAR
				    && (ak == 0x1C || ak == 0x1D
				    || ak == 0x1E || ak == 0x1F
				    || ak == 0x0B || ak == 0x0C)) {
					/* Arrow/Page key repeat — scroll */
					handle_key_down(&event);
				} else {
					/* Text repeat in address bar */
					GrafPtr save;

					GetPort(&save);
					SetPort(g_window);
					browser_key(g_window, &event);
					SetPort(save);
				}
			}
			break;
		case mouseDown:
#ifdef GEOMYS_DOWNLOAD
			/* Check for download dialog Stop button */
			if (g_dl_dialog &&
			    IsDialogEvent(&event)) {
				DialogPtr dlg;
				short item;

				if (DialogSelect(&event,
				    &dlg, &item) &&
				    dlg == g_dl_dialog &&
				    item == 1) {
					do_cancel_loading();
				}
				break;
			}
#endif
			handle_mouse_down(&event);
			break;
		case updateEvt:
#ifdef GEOMYS_DOWNLOAD
			/* Let Dialog Manager handle dialog updates */
			if (g_dl_dialog &&
			    IsDialogEvent(&event)) {
				DialogPtr dlg;
				short item;
				DialogSelect(&event, &dlg, &item);
				break;
			}
#endif
			handle_update(&event);
			break;
		case activateEvt:
			handle_activate(&event);
			break;
		case app4Evt:
			if (HiWord(event.message) & (1 << 8)) {
				if (event.message & 1) {
					/* Resume */
					g_suspended = false;
					if (g_nm_posted) {
						NMRemove(&g_nm_rec);
						g_nm_posted = false;
					}
				} else {
					g_suspended = true;
				}
			}
			break;
		case kHighLevelEvent:
			AEProcessAppleEvent(&event);
			break;
		}
	}

#ifdef GEOMYS_CACHE
	cache_cleanup();
#endif
#ifdef GEOMYS_OFFSCREEN
	offscreen_cleanup();
#endif

	/* Destroy all remaining sessions (closes connections,
	 * disposes TE/controls/windows, frees gopher data) */
	{
		short i;

		for (i = GEOMYS_MAX_WINDOWS - 1; i >= 0; i--) {
			BrowserSession *s = session_get(i);
			if (s)
				session_destroy(s);
		}
	}

	/* Remove any pending notification */
	if (g_nm_posted) {
		NMRemove(&g_nm_rec);
		g_nm_posted = false;
	}

	/* Restore system key repeat settings */
	LMSetKeyThresh(saved_key_thresh);
	LMSetKeyRepThresh(saved_key_rep_thresh);

	ExitToShell();
}

/* Forward declaration for keyboard refresh shortcut */
static void handle_nav_button(short btn_id);

#ifdef GEOMYS_DOWNLOAD
/*
 * Download progress dialog — open, update, close.
 * Uses SetDialogItemText to update byte count directly
 * rather than ParamText (which would overwrite filename).
 */
static Str255 g_dl_pname;  /* filename for ParamText ^0 */

static void
dl_progress_open(void)
{
	char cname[64];
	short len;
	Str255 pbytes;

	if (g_dl_dialog)
		return;

	/* Convert Pascal dl_filename to C string */
	len = g_gopher.dl_filename[0];
	if (len > 63) len = 63;
	memcpy(cname, g_gopher.dl_filename + 1, len);
	cname[len] = '\0';

	c2pstr(g_dl_pname, cname);
	c2pstr(pbytes, "0");
	ParamText(g_dl_pname, pbytes, "\p", "\p");

	g_dl_dialog = GetNewDialog(DLOG_DL_PROGRESS_ID,
	    0L, (WindowPtr)-1);
	if (g_dl_dialog) {
		center_dialog_on_screen(g_dl_dialog);
		ShowWindow((WindowPtr)g_dl_dialog);
		DrawDialog(g_dl_dialog);
	}
}

static void
dl_progress_update(long bytes)
{
	Str255 pbytes;
	char buf[32];
	GrafPtr save;
	short item_type;
	Handle item_h;
	Rect item_rect;

	if (!g_dl_dialog)
		return;

	/* Update byte count — only redraw the text item,
	 * not the entire dialog, to avoid flicker */
	snprintf(buf, sizeof(buf), "%ld bytes received",
	    bytes);
	c2pstr(pbytes, buf);

	GetDialogItem(g_dl_dialog, 3, &item_type,
	    &item_h, &item_rect);
	SetDialogItemText(item_h, pbytes);

	GetPort(&save);
	SetPort((WindowPtr)g_dl_dialog);
	EraseRect(&item_rect);
	TETextBox(buf, (long)strlen(buf),
	    &item_rect, teFlushDefault);
	SetPort(save);
}

static void
dl_progress_close(void)
{
	if (g_dl_dialog) {
		DisposeDialog(g_dl_dialog);
		g_dl_dialog = 0L;
	}
}
#endif

/*
 * Cancel any in-progress page load or download.
 * Used by Cmd-., Stop button, and Go > Stop menu.
 */
void
do_cancel_loading(void)
{
	char status[80];

	if (g_app_state != APP_STATE_LOADING ||
	    !g_gopher.receiving)
		return;

	conn_close(&g_gopher.conn);
	g_gopher.receiving = false;

#ifdef GEOMYS_DOWNLOAD
	dl_progress_close();
	if (g_gopher.page_type == PAGE_DOWNLOAD ||
	    g_gopher.page_type == PAGE_IMAGE) {
		/* Close and delete partial download file */
		if (g_gopher.dl_refnum) {
			FSClose(g_gopher.dl_refnum);
			FlushVol(0L, g_gopher.dl_vrefnum);
			FSDelete(g_gopher.dl_filename,
			    g_gopher.dl_vrefnum);
			g_gopher.dl_refnum = 0;
		}

		snprintf(status, sizeof(status),
		    "Stopped \xD0 %ld bytes",
		    g_gopher.dl_written);

		g_gopher.dl_written = 0;
		g_gopher.dl_error = false;
		g_gopher.dl_vrefnum = 0;
		g_gopher.dl_filename[0] = 0;
		g_gopher.img_header_len = 0;
		g_gopher.img_sniffed = false;

		/* Restore page and title from before download */
		g_gopher.page_type = g_gopher.dl_prev_page;
		{
			const HistoryEntry *he;

			he = history_current();
			if (he && he->title[0])
				set_wtitlef(g_window, "%s",
				    he->title);
			else
				set_wtitlef(g_window, "%s",
				    g_gopher.cur_host);
		}
	} else
#endif
	if (g_gopher.page_type == PAGE_DIRECTORY) {
		snprintf(status, sizeof(status),
		    "Stopped \xD0 %d items",
		    g_gopher.item_count);
	} else {
		snprintf(status, sizeof(status),
		    "Stopped \xD0 %ld bytes",
		    g_gopher.text_len);
	}

	g_app_state = APP_STATE_IDLE;
	InitCursor();
	browser_set_status(status);
	update_nav_buttons();

	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		content_mark_all_dirty();
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw(g_window);
		SetPort(save);
	}
}

/*
 * handle_key_down - process keyDown events
 */
static void
handle_key_down(EventRecord *event)
{
	char key;

	key = event->message & charCodeMask;

	if (event->modifiers & cmdKey) {
		/* Cmd-[ = back, Cmd-] = forward */
		if (key == '[') {
			if (history_can_back()) {
				history_set_scroll(
				    content_get_scroll_pos());
				navigate_history_entry(
				    history_back(), -1);
			}
			return;
		}
		if (key == ']') {
			if (history_can_forward()) {
				history_set_scroll(
				    content_get_scroll_pos());
				navigate_history_entry(
				    history_forward(), 1);
			}
			return;
		}

		/* Cmd-R, Cmd-L, Cmd-. handled via Go menu
		 * through MenuKey below */

		update_menus();
		handle_menu(MenuKey(key));
		return;
	}

	/* Return/Enter in address bar — navigate */
	if (key == '\r' || key == '\n' || key == 0x03) {
		char url[300];

		browser_get_url(url, sizeof(url));
		if (url[0])
			do_navigate_url(url);
		return;
	}

	/* Tab / Shift-Tab — cycle focus between address bar and content */
	if (key == 0x09) {
		if (event->modifiers & shiftKey) {
			if (browser_get_focus() == FOCUS_CONTENT) {
				browser_set_focus(FOCUS_ADDR_BAR);
				browser_activate(true);
			} else {
				browser_set_focus(FOCUS_CONTENT);
				browser_activate(false);
			}
		} else {
			if (browser_get_focus() == FOCUS_ADDR_BAR) {
				browser_set_focus(FOCUS_CONTENT);
				browser_activate(false);
			} else {
				browser_set_focus(FOCUS_ADDR_BAR);
				browser_activate(true);
			}
		}
		return;
	}

	/* Arrow / Page / Home / End — scroll content when not in addr bar */
	if (browser_get_focus() != FOCUS_ADDR_BAR) {
		/* Up/Down: link selection on directory pages, scroll on text */
		if (key == 0x1E || key == 0x1F) {
			if (g_gopher.page_type == PAGE_DIRECTORY) {
				if (key == 0x1E)
					content_select_prev(g_window);
				else
					content_select_next(g_window);
				return;
			}
		}

		/* Return: follow selected link */
		if ((key == '\r' || key == '\n' || key == 0x03) &&
		    content_get_selected_row() >= 0 &&
		    g_gopher.page_type == PAGE_DIRECTORY) {
			short sel_row = content_get_selected_row();

			content_clear_kbd_selection(g_window);
			content_click_row(g_window, &g_gopher,
			    sel_row);
			return;
		}

		switch (key) {
		case 0x1C:  /* Left arrow */
			content_hscroll_by(-content_hscroll_step());
			return;
		case 0x1D:  /* Right arrow */
			content_hscroll_by(content_hscroll_step());
			return;
		case 0x1E:  /* Up arrow */
			content_vscroll_by(-1);
			return;
		case 0x1F:  /* Down arrow */
			content_vscroll_by(1);
			return;
		case 0x0B:  /* Page Up */
			content_vscroll_by(
			    -content_visible_rows());
			return;
		case 0x0C:  /* Page Down */
			content_vscroll_by(
			    content_visible_rows());
			return;
		case 0x01:  /* Home — top of document */
			content_set_scroll_pos(0);
			return;
		case 0x04:  /* End — bottom of document */
			content_set_scroll_pos(32767);
			return;
		}
	}

	/* Pass to address bar */
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_key(g_window, event);
		SetPort(save);
	}
}

void
do_navigate_url(const char *url)
{
	do_navigate_url_titled(url, 0L);
}

/*
 * Navigate to a gopher:// URL string with optional page title.
 * If title is NULL, uses the hostname.
 */
void
do_navigate_url_titled(const char *url, const char *title)
{
	char host[64];
	short port;
	char type;
	char selector[256];
	char uri[300];

	if (!gopher_parse_uri(url, host, sizeof(host),
	    &port, &type, selector, sizeof(selector)))
		return;

	/* Save scroll position before resetting */
	history_set_scroll(content_get_scroll_pos());

	g_app_state = APP_STATE_LOADING;
	content_scroll_to_top();

	/* Show loading state in title bar and status bar */
	set_wtitlef(g_window, "Loading %.50s\311", host);
	snprintf(uri, sizeof(uri), "Loading %.50s\311", host);
	browser_set_status(uri);
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	}

	SetCursor(*GetCursor(watchCursor));
	if (!gopher_navigate(&g_gopher, host, port, type, selector)) {
		GrafPtr save;
		const HistoryEntry *he;

		InitCursor();

		/* Navigate failed — old page preserved, force redraw */
		g_app_state = APP_STATE_IDLE;

		/* Restore title bar from current history entry */
		he = history_current();
		if (he && he->title[0])
			set_wtitlef(g_window, "%s", he->title);
		else if (he)
			set_wtitlef(g_window, "%s", he->host);
		else
			set_wtitlef(g_window, "Geomys");

		browser_set_status("Connection failed");
		GetPort(&save);
		SetPort(g_window);
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	} else {
		/* Success — push to history.
		 * Scroll was saved before content_scroll_to_top.
		 * Invalidate forward cache entries since
		 * history_push clears forward history. */
#ifdef GEOMYS_CACHE
		cache_invalidate_from(active_session->id,
		    history_current_index() + 1);
#endif
		history_push(host, port, type, selector,
		    (title && title[0]) ? title : host,
		    0L);
		update_nav_buttons();
	}
}

/*
 * Refresh the current page — re-fetches without
 * pushing a new history entry.
 */
void
do_refresh(void)
{
	if (!g_gopher.cur_host[0] ||
	    g_app_state != APP_STATE_IDLE)
		return;

#ifdef GEOMYS_CACHE
	cache_invalidate(active_session->id,
	    history_current_index());
#endif

	g_app_state = APP_STATE_LOADING;
	browser_set_status("Loading\311");
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	}

	set_wtitlef(g_window, "Loading %s\311",
	    g_gopher.cur_host);
	SetCursor(*GetCursor(watchCursor));

	if (!gopher_navigate(&g_gopher,
	    g_gopher.cur_host, g_gopher.cur_port,
	    g_gopher.cur_type, g_gopher.cur_selector)) {
		GrafPtr save;

		g_app_state = APP_STATE_IDLE;
		InitCursor();
		browser_set_status("Connection failed");

		GetPort(&save);
		SetPort(g_window);
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	} else {
		update_nav_buttons();
	}
}



/*
 * Search dialog — shown when clicking a Type 7 item
 */
void
do_search_dialog(const char *title, const char *host,
    short port, const char *selector)
{
	DialogPtr dlg;
	short item;
	short item_type;
	Handle item_h;
	Rect item_rect;
	Str255 pstr;
	char label[100];

	/* Deactivate address bar so modal dialog gets keystrokes */
	browser_activate(false);

	/* Create dialog behind all windows so it doesn't
	 * draw at the DLOG resource position first —
	 * avoids white rectangle artifact on color displays */
	dlg = GetNewDialog(DLOG_SEARCH_ID, 0L, 0L);
	if (!dlg) {
		browser_activate(true);
		return;
	}
	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	/* Set label with search item name */
	snprintf(label, sizeof(label), "Search %.60s:", title);
	c2pstr(pstr, label);
	GetDialogItem(dlg, 3, &item_type, &item_h, &item_rect);
	SetDialogItemText(item_h, pstr);

	setup_default_button_outline(dlg, 5);
	SelectDialogItemText(dlg, 4, 0, 0);

	/* Loop until Search or Cancel button clicked */
	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter, &item);
	} while (item != 1 && item != 2);

	if (item == 1) {
		char query[256];
		char full_sel[512];

		GetDialogItem(dlg, 4, &item_type, &item_h,
		    &item_rect);
		GetDialogItemText(item_h, pstr);
		{
			short len = pstr[0];
			if (len >= (short)sizeof(query))
				len = sizeof(query) - 1;
			memcpy(query, pstr + 1, len);
			query[len] = '\0';
		}

		DisposeDialog(dlg);
		browser_activate(true);

		/* Invalidate window behind dismissed dialog.
		 * InvalRect queues an updateEvt so handle_update
		 * does a full redraw (content_mark_all_dirty +
		 * browser_draw + content_draw). */
		{
			GrafPtr save;
			GetPort(&save);
			SetPort(g_window);
			InvalRect(&g_window->portRect);
			SetPort(save);
		}

		if (query[0]) {
			char search_title[100];
			char uri[300];

			/* Append query to selector with tab */
			snprintf(full_sel, sizeof(full_sel),
			    "%s\t%s", selector, query);

			/* Save scroll before resetting */
			history_set_scroll(
			    content_get_scroll_pos());

			g_app_state = APP_STATE_LOADING;
			content_scroll_to_top();

			/* Show loading state in title bar
			 * and status bar */
			set_wtitlef(g_window, "Loading %.50s\311",
			    host);
			snprintf(uri, sizeof(uri),
			    "Loading %.50s\311", host);
			browser_set_status(uri);

			SetCursor(*GetCursor(watchCursor));
			if (gopher_navigate(&g_gopher, host, port,
			    GOPHER_DIRECTORY, full_sel)) {
#ifdef GEOMYS_CACHE
				cache_invalidate_from(
				    active_session->id,
				    history_current_index() + 1);
#endif
				/* Push to history with query preserved */
				snprintf(search_title,
				    sizeof(search_title),
				    "Search: %s", query);
				history_push(host, port,
				    GOPHER_SEARCH, selector,
				    search_title, query);
				update_nav_buttons();
			} else {
				InitCursor();

				/* Navigate failed — restore state */
				g_app_state = APP_STATE_IDLE;

				/* Restore title bar */
				{
					const HistoryEntry *he =
					    history_current();
					if (he && he->title[0])
						set_wtitlef(g_window,
						    "%s", he->title);
					else if (he)
						set_wtitlef(g_window,
						    "%s", he->host);
					else
						set_wtitlef(g_window,
						    "Geomys");
				}

				browser_set_status("Search failed");
				{
					GrafPtr save;
					GetPort(&save);
					SetPort(g_window);
					content_draw(g_window);
					content_update_scroll(g_window);
					browser_draw_status(g_window);
					SetPort(save);
				}
			}
		}
	} else {
		DisposeDialog(dlg);
		browser_activate(true);

		/* Invalidate window behind dismissed dialog */
		{
			GrafPtr save;
			GetPort(&save);
			SetPort(g_window);
			InvalRect(&g_window->portRect);
			SetPort(save);
		}
	}
}

#ifdef GEOMYS_CLIPBOARD
/*
 * Find in Page dialog — Edit > Find (Cmd+F)
 */
void
do_find_dialog(void)
{
	DialogPtr dlg;
	short item;
	short item_type;
	Handle item_h;
	Rect item_rect;
	Str255 pstr;
	const char *prev_query;

	/* Deactivate address bar so modal dialog gets keystrokes */
	browser_activate(false);

	dlg = GetNewDialog(DLOG_FIND_ID, 0L, 0L);
	if (!dlg) {
		browser_activate(true);
		return;
	}
	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	/* Pre-fill with previous query if any */
	prev_query = content_find_query();
	if (prev_query && prev_query[0]) {
		c2pstr(pstr, prev_query);
		GetDialogItem(dlg, 4, &item_type, &item_h,
		    &item_rect);
		SetDialogItemText(item_h, pstr);
	}

	setup_default_button_outline(dlg, 5);
	SelectDialogItemText(dlg, 4, 0, 32767);

	/* Loop until Find or Cancel button clicked */
	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter,
		    &item);
	} while (item != 1 && item != 2);

	if (item == 1) {
		char query[64];

		GetDialogItem(dlg, 4, &item_type, &item_h,
		    &item_rect);
		GetDialogItemText(item_h, pstr);
		{
			short len = pstr[0];
			if (len >= (short)sizeof(query))
				len = sizeof(query) - 1;
			memcpy(query, pstr + 1, len);
			query[len] = '\0';
		}

		DisposeDialog(dlg);
		browser_activate(true);

		/* Invalidate window behind dismissed dialog */
		{
			GrafPtr save;
			GetPort(&save);
			SetPort(g_window);
			InvalRect(&g_window->portRect);
			SetPort(save);
		}

		if (query[0])
			content_find(query);
	} else {
		DisposeDialog(dlg);
		browser_activate(true);

		/* Invalidate window behind dismissed dialog */
		{
			GrafPtr save;
			GetPort(&save);
			SetPort(g_window);
			InvalRect(&g_window->portRect);
			SetPort(save);
		}
	}
}
#endif /* GEOMYS_CLIPBOARD */

/*
 * Show a message for non-navigable Gopher types
 */
void
do_type_message(char type, const char *display,
    const char *host, short port)
{
	char msg[200];
	Str255 pmsg;

	(void)host;
	(void)port;

	switch (type) {
	case GOPHER_TELNET:
	case GOPHER_TN3270:
		snprintf(msg, sizeof(msg),
		    "\"%.60s\" requires a telnet client. "
		    "Use Flynn to connect.", display);
		break;
	case GOPHER_BINHEX:
	case GOPHER_DOS:
	case GOPHER_UUENCODE:
	case GOPHER_BINARY:
	case GOPHER_DOC:
		snprintf(msg, sizeof(msg),
		    "File downloading is not supported "
		    "in this version of Geomys.");
		break;
	case GOPHER_GIF:
	case GOPHER_IMAGE:
	case GOPHER_PNG:
		snprintf(msg, sizeof(msg),
		    "Image display is not supported "
		    "in this version of Geomys.");
		break;
	case GOPHER_RTF:
		snprintf(msg, sizeof(msg),
		    "RTF document display is not supported "
		    "in this version of Geomys.");
		break;
	case GOPHER_SOUND:
		snprintf(msg, sizeof(msg),
		    "Sound playback is not supported "
		    "in this version of Geomys.");
		break;
	default:
		snprintf(msg, sizeof(msg),
		    "This item type is not supported "
		    "in this version of Geomys.");
		break;
	}

	c2pstr(pmsg, msg);
	ParamText(pmsg, "\p", "\p", "\p");
	NoteAlert(128, 0L);
}

/*
 * Show a dialog with an external URL from a type h item.
 * The URL is placed in an editable text field so the user
 * can Cmd-C to copy it.
 */
void
do_html_url_dialog(const char *url, const char *display)
{
	DialogPtr dlg;
	short item;
	short item_type;
	Handle item_h;
	Rect item_rect;
	Str255 pstr;

	(void)display;

	browser_activate(false);

	dlg = GetNewDialog(DLOG_HTML_URL_ID, 0L, 0L);
	if (!dlg) {
		browser_activate(true);
		return;
	}
	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	/* Set URL in the EditText field */
	c2pstr(pstr, url);
	GetDialogItem(dlg, 4, &item_type, &item_h, &item_rect);
	SetDialogItemText(item_h, pstr);

	setup_default_button_outline(dlg, 5);
	SelectDialogItemText(dlg, 4, 0, 32767);

	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter, &item);
	} while (item != 1 && item != 2);

	DisposeDialog(dlg);
	browser_activate(true);

	/* Invalidate window behind dismissed dialog */
	{
		GrafPtr save;
		GetPort(&save);
		SetPort(g_window);
		InvalRect(&g_window->portRect);
		SetPort(save);
	}
}

static void
update_nav_buttons(void)
{
	GrafPtr save;

	browser_set_button_state(NAV_BTN_BACK,
	    history_can_back() ? BTN_ENABLED : BTN_DISABLED);
	browser_set_button_state(NAV_BTN_FORWARD,
	    history_can_forward() ? BTN_ENABLED : BTN_DISABLED);

	/* Update action button: stop during loading,
	 * go if URL was edited, refresh otherwise */
	if (g_app_state == APP_STATE_LOADING &&
	    g_gopher.receiving) {
		browser_set_action_state(ACTION_STOP);
	} else {
		char url[300];
		char cur[300];

		browser_get_url(url, sizeof(url));
		gopher_build_uri(cur, sizeof(cur),
		    g_gopher.cur_host, g_gopher.cur_port,
		    g_gopher.cur_type,
		    g_gopher.cur_selector);
		if (strcmp(url, cur) != 0 &&
		    url[0] != '\0')
			browser_set_action_state(ACTION_GO);
		else
			browser_set_action_state(ACTION_REFRESH);
	}

	GetPort(&save);
	SetPort(g_window);
	browser_draw(g_window);
	SetPort(save);
}

/*
 * Navigate to a history entry.
 * direction: -1 = came from back, +1 = came from forward
 */
void
navigate_history_entry(const HistoryEntry *e, short direction)
{
	char uri[300];

	if (!e)
		return;

#ifdef GEOMYS_CACHE
	/* Try cache first for instant back/forward */
	if (cache_retrieve(active_session->id,
	    history_current_index(), &g_gopher)) {
		GrafPtr save;

		/* Cancel any in-progress loading — the active
		 * connection would otherwise append data to the
		 * restored cached page */
		if (g_gopher.conn.state != CONN_STATE_IDLE)
			conn_close(&g_gopher.conn);
		g_app_state = APP_STATE_IDLE;

		content_set_page(&g_gopher);

		gopher_build_uri(uri, sizeof(uri), e->host, e->port,
		    e->type, e->selector);
		browser_set_url(uri);

		/* Copy host/port/selector/type into gopher state */
		strncpy(g_gopher.cur_host, e->host,
		    sizeof(g_gopher.cur_host) - 1);
		g_gopher.cur_host[sizeof(g_gopher.cur_host) - 1] = '\0';
		g_gopher.cur_port = e->port;
		g_gopher.cur_type = e->type;
		strncpy(g_gopher.cur_selector, e->selector,
		    sizeof(g_gopher.cur_selector) - 1);
		g_gopher.cur_selector[sizeof(g_gopher.cur_selector) - 1] = '\0';

		browser_set_status("Done (cached)");

		if (e->title[0])
			set_wtitlef(g_window, "%s", e->title);
		else
			set_wtitlef(g_window, "%s", e->host);

		update_nav_buttons();

		GetPort(&save);
		SetPort(g_window);
		content_recalc_width(g_window);
		content_update_scroll(g_window);
		content_set_scroll_pos(e->scroll_pos);
		content_draw(g_window);
		browser_draw_status(g_window);
		browser_draw(g_window);
		SetPort(save);
		return;
	}
#endif

	g_app_state = APP_STATE_LOADING;
	g_pending_scroll = e->scroll_pos;
	content_scroll_to_top();

	set_wtitlef(g_window, "Loading %.50s\311", e->host);
	snprintf(uri, sizeof(uri), "Loading %.50s\311", e->host);
	browser_set_status(uri);

	gopher_build_uri(uri, sizeof(uri), e->host, e->port,
	    e->type, e->selector);
	browser_set_url(uri);

	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_draw(g_window);
		SetPort(save);
	}

	/* Navigate without pushing to history.
	 * For search entries, reconstruct selector\tquery */
	{
		char nav_sel[512];
		const char *sel = e->selector;

		if (e->type == GOPHER_SEARCH && e->query[0]) {
			snprintf(nav_sel, sizeof(nav_sel),
			    "%s\t%s", e->selector, e->query);
			sel = nav_sel;
		}

		SetCursor(*GetCursor(watchCursor));
		if (!gopher_navigate(&g_gopher, e->host, e->port,
		    (e->type == GOPHER_SEARCH) ?
		    GOPHER_DIRECTORY : e->type, sel)) {
		GrafPtr save;

		InitCursor();

		/* Undo the history move */
		if (direction < 0)
			history_undo_back();
		else
			history_undo_forward();

		g_app_state = APP_STATE_IDLE;
		g_pending_scroll = -1;

		/* Restore title bar */
		{
			const HistoryEntry *cur = history_current();
			if (cur && cur->title[0])
				set_wtitlef(g_window, "%s", cur->title);
			else if (cur)
				set_wtitlef(g_window, "%s", cur->host);
			else
				set_wtitlef(g_window, "Geomys");
		}

		browser_set_status("Connection failed");
		GetPort(&save);
		SetPort(g_window);
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw_status(g_window);
		SetPort(save);
		}
	}

	update_nav_buttons();
}

/*
 * navigate_history_to - Jump directly to a history position.
 * Used by Window menu history list.
 */
void
navigate_history_to(short index)
{
	const HistoryEntry *e;
	short cur = history_current_index();
	short direction;

	e = history_get(index);
	if (!e || index == cur)
		return;

	history_set_scroll(content_get_scroll_pos());

	/* Move position to target */
	direction = (index < cur) ? -1 : 1;
	while (history_current_index() != index) {
		if (direction < 0)
			history_back();
		else
			history_forward();
	}

	navigate_history_entry(e, direction);
}

/*
 * Home Page dialog (Options > Home Page)
 */
void
do_home_page_dialog(void)
{
	DialogPtr dlg;
	short item;
	short item_type;
	Handle item_h;
	Rect item_rect;
	Str255 pstr;
	Boolean use_blank;

	dlg = GetNewDialog(DLOG_HOME_PAGE_ID, 0L, 0L);
	if (!dlg)
		return;
	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	/* Pre-fill URL */
	c2pstr(pstr, g_prefs.home_url);
	GetDialogItem(dlg, 4, &item_type, &item_h, &item_rect);
	SetDialogItemText(item_h, pstr);

	/* Set blank checkbox */
	use_blank = (g_prefs.home_url[0] == '\0');
	GetDialogItem(dlg, 5, &item_type, &item_h, &item_rect);
	SetControlValue((ControlHandle)item_h, use_blank ? 1 : 0);

	setup_default_button_outline(dlg, 6);

	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter, &item);

		/* Toggle blank checkbox */
		if (item == 5) {
			GetDialogItem(dlg, 5, &item_type, &item_h,
			    &item_rect);
			use_blank = !GetControlValue(
			    (ControlHandle)item_h);
			SetControlValue((ControlHandle)item_h,
			    use_blank ? 1 : 0);
		}
	} while (item != 1 && item != 2);

	if (item == 1) {
		if (use_blank) {
			g_prefs.home_url[0] = '\0';
		} else {
			short len;

			GetDialogItem(dlg, 4, &item_type, &item_h,
			    &item_rect);
			GetDialogItemText(item_h, pstr);
			len = pstr[0];
			if (len >= (short)sizeof(g_prefs.home_url))
				len = sizeof(g_prefs.home_url) - 1;
			memcpy(g_prefs.home_url, pstr + 1, len);
			g_prefs.home_url[len] = '\0';
		}
		prefs_save(&g_prefs);
	}

	DisposeDialog(dlg);
}

static void
handle_nav_button(short btn_id)
{
	switch (btn_id) {
	case NAV_BTN_BACK:
		history_set_scroll(content_get_scroll_pos());
		navigate_history_entry(history_back(), -1);
		break;
	case NAV_BTN_FORWARD:
		history_set_scroll(content_get_scroll_pos());
		navigate_history_entry(history_forward(), 1);
		break;
	case NAV_BTN_HOME:
		if (g_prefs.home_url[0])
			do_navigate_url(g_prefs.home_url);
		break;
	}
}

/*
 * Handle action button click — dynamic stop/go/refresh.
 */
static void
handle_action_button(void)
{
	switch (browser_get_action_state()) {
	case ACTION_STOP:
		do_cancel_loading();
		break;
	case ACTION_GO: {
		char url[300];
		browser_get_url(url, sizeof(url));
		if (url[0])
			do_navigate_url(url);
		break;
	}
	case ACTION_REFRESH:
		do_refresh();
		break;
	}
}

static void
handle_mouse_down(EventRecord *event)
{
	WindowPtr win;
	short part;

	part = FindWindow(event->where, &win);

	switch (part) {
	case inMenuBar:
		update_menus();
		handle_menu(MenuSelect(event->where));
		break;
	case inSysWindow:
		SystemClick(event, win);
		break;
	case inDrag:
		DragWindow(win, event->where,
		    &qd.screenBits.bounds);
		break;
	case inGoAway:
		if (TrackGoAway(win, event->where)) {
			BrowserSession *s;

			s = session_from_window(win);
			if (s)
				session_destroy_and_fixup(s);
			else
				g_running = false;
		}
		break;
	case inZoomIn:
	case inZoomOut:
		if (TrackBox(win, event->where, part)) {
			GrafPtr save;
#if GEOMYS_MAX_WINDOWS > 1
			BrowserSession *gs, *orig;

			gs = session_from_window(win);
			orig = active_session;
			if (gs && gs != active_session)
				session_switch_to(gs);
#endif
			GetPort(&save);
			SetPort(win);
			EraseRect(&win->portRect);
			ZoomWindow(win, part, win == FrontWindow());
#ifdef GEOMYS_OFFSCREEN
			offscreen_resize(win);
#endif
			content_resize(win);
			InvalRect(&win->portRect);
			SetPort(save);
#if GEOMYS_MAX_WINDOWS > 1
			if (gs)
				session_save_state(gs);
			if (gs && gs != orig && orig)
				session_switch_to(orig);
#endif
		}
		break;
	case inGrow: {
		long new_size;
		Rect limit_rect;

		SetRect(&limit_rect, 200, 150,
		    qd.screenBits.bounds.right,
		    qd.screenBits.bounds.bottom);
		new_size = GrowWindow(win, event->where,
		    &limit_rect);
		if (new_size != 0) {
			GrafPtr save;
#if GEOMYS_MAX_WINDOWS > 1
			BrowserSession *gs, *orig;

			gs = session_from_window(win);
			orig = active_session;
			if (gs && gs != active_session)
				session_switch_to(gs);
#endif
			SizeWindow(win, LoWord(new_size),
			    HiWord(new_size), true);
#ifdef GEOMYS_OFFSCREEN
			offscreen_resize(win);
#endif
			GetPort(&save);
			SetPort(win);
			content_resize(win);
			InvalRect(&win->portRect);
			SetPort(save);
#if GEOMYS_MAX_WINDOWS > 1
			/* Save resized state so session switch
			 * restores correct layout */
			if (gs)
				session_save_state(gs);
			if (gs && gs != orig && orig)
				session_switch_to(orig);
#endif
		}
		break;
	}
	case inContent:
		if (win != FrontWindow()) {
			SelectWindow(win);
		} else if (session_from_window(win)) {
			Point local_pt;
			GrafPtr save;
			short click_result;

			GetPort(&save);
			SetPort(win);
			local_pt = event->where;
			GlobalToLocal(&local_pt);

			/* Check browser chrome first */
			click_result = browser_click(win, local_pt);
			if (click_result >= 0) {
				/* Nav button clicked */
				handle_nav_button(click_result);
			} else if (click_result == -4) {
				/* Action button (stop/go/refresh) */
				handle_action_button();
			} else if (click_result == -1) {
				/* Check scroll bars first */
				ControlHandle hit_ctl;
				short ctl_part;

				ctl_part = FindControl(local_pt,
				    win, &hit_ctl);

				if (ctl_part &&
				    hit_ctl ==
				    content_get_scrollbar()) {
					content_scroll_click(
					    win, local_pt,
					    ctl_part);
				} else if (ctl_part &&
				    hit_ctl ==
				    content_get_hscrollbar()) {
					content_hscroll_click(
					    win, local_pt,
					    ctl_part);
				} else {
					/* Content area click */
					content_click(win,
					    local_pt,
					    &g_gopher);
				}
			}
			/* -2 = address bar, -3 = disabled btn — handled */

			SetPort(save);
		}
		break;
	}
}

static void
handle_update(EventRecord *event)
{
	WindowPtr win;
	GrafPtr old_port;
	BrowserSession *s;

	win = (WindowPtr)event->message;
	s = session_from_window(win);

	GetPort(&old_port);
	SetPort(win);
	BeginUpdate(win);

	if (s) {
#if GEOMYS_MAX_WINDOWS > 1
		BrowserSession *orig = active_session;

		if (s != active_session)
			session_switch_to(s);
#endif
		content_mark_all_dirty();
		browser_draw(win);
		content_draw(win);
		content_update_scroll(win);
		DrawControls(win);

		/* Draw grow box over scrollbar overlap. */
		{
			Rect clip_r;
			RgnHandle save_clip;

			save_clip = NewRgn();
			GetClip(save_clip);
			SetRect(&clip_r,
			    win->portRect.right - SCROLLBAR_WIDTH,
			    win->portRect.bottom - SCROLLBAR_WIDTH,
			    win->portRect.right + 1,
			    win->portRect.bottom + 1);
			ClipRect(&clip_r);
			EraseRect(&clip_r);
			DrawGrowIcon(win);
			SetClip(save_clip);
			DisposeRgn(save_clip);
		}

#if GEOMYS_MAX_WINDOWS > 1 
		if (s != orig && orig)
			session_switch_to(orig);
#endif
	} else {
		EraseRect(&win->portRect);
	}

	EndUpdate(win);
	SetPort(old_port);
}

static void
handle_activate(EventRecord *event)
{
	WindowPtr win;
	BrowserSession *s;

	win = (WindowPtr)event->message;
	s = session_from_window(win);
	if (!s)
		return;

	if (event->modifiers & activeFlag) {
#if GEOMYS_MAX_WINDOWS > 1 
		session_switch_to(s);
#endif
		browser_activate(true);
#ifdef GEOMYS_CLIPBOARD
		content_activate(win, true);
#endif
		/* Restore scrollbar and force full redraw to
		 * ensure correct content after session switch */
		{
			GrafPtr save;

			GetPort(&save);
			SetPort(win);
			content_update_scroll(win);
#if GEOMYS_MAX_WINDOWS > 1
			/* Ensure back/forward buttons reflect
			 * this session's history state */
			update_nav_buttons();

			/* Force full redraw with correct session
			 * state — prevents stale content from
			 * previous session showing through */
			InvalRect(&win->portRect);
#endif
			SetPort(save);
		}
	} else {
#if GEOMYS_MAX_WINDOWS > 1
		if (s == active_session) {
#endif
			browser_activate(false);
#ifdef GEOMYS_CLIPBOARD
			content_activate(win, false);
#endif
			/* Dim scrollbars on deactivation (per HIG) */
			{
				ControlHandle sb;

				sb = content_get_scrollbar();
				if (sb)
					HiliteControl(sb, 255);
				sb = content_get_hscrollbar();
				if (sb)
					HiliteControl(sb, 255);
			}
#if GEOMYS_MAX_WINDOWS > 1
			session_save_state(s);
		} else {
			/* active_session already switched (e.g.,
			 * do_new_window). Use session's stored
			 * handles for deactivation visuals. */
			GrafPtr sp;

			GetPort(&sp);
			SetPort(win);
			if (s->addr_te)
				TEDeactivate(s->addr_te);
			if (s->scrollbar)
				HiliteControl(s->scrollbar, 255);
			if (s->hscrollbar)
				HiliteControl(s->hscrollbar, 255);
			SetPort(sp);
		}
#endif
	}
}
