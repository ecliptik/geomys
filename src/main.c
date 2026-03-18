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

/* Globals */
Boolean g_running = true;
Boolean g_suspended = false;
WindowPtr g_window = 0L;
short g_app_state = APP_STATE_IDLE;
GopherState g_gopher;

/* Forward declarations */
static void init_toolbox(void);
static void init_apple_events(void);
static void create_window(void);
static void main_event_loop(void);
static void handle_mouse_down(EventRecord *event);
static void handle_key_down(EventRecord *event);
static void handle_update(EventRecord *event);
static void handle_activate(EventRecord *event);
static void draw_page(void);
/* do_navigate_url and do_open_url_dialog declared in main.h */
static void handle_nav_button(short btn_id);

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
	init_menus();
	create_window();

	/* Initialize browser chrome */
	{
		GrafPtr save;
		GetPort(&save);
		SetPort(g_window);
		browser_init(g_window);
		SetPort(save);
	}

	update_menus();

	/* Initialize Gopher engine and navigate to home page */
	gopher_init(&g_gopher);
	browser_set_status("Connecting to sdf.org\311");
	browser_set_url(DEFAULT_HOME_URL);
	g_app_state = APP_STATE_LOADING;
	gopher_navigate(&g_gopher, DEFAULT_HOME_HOST,
	    GOPHER_DEFAULT_PORT, GOPHER_DIRECTORY, "");

	main_event_loop();
	return 0;
}

static void
init_toolbox(void)
{
	SetApplLimit(LMGetApplLimit() - (1024 * 8));
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();
	MaxApplZone();
}

static void
create_window(void)
{
	Rect bounds;

	SetRect(&bounds, 2, 42, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2);
	g_window = NewWindow(0L, &bounds, "\pGeomys", true,
	    documentProc, (WindowPtr)-1L, true, 0L);
}

static void
main_event_loop(void)
{
	EventRecord event;
	long wait_ticks;

	while (g_running) {
		if (g_suspended)
			wait_ticks = 60L;
		else if (g_app_state == APP_STATE_LOADING)
			wait_ticks = 0L;
		else
			wait_ticks = 10L;

		WaitNextEvent(everyEvent, &event, wait_ticks, 0L);

		switch (event.what) {
		case nullEvent:
			/* Poll Gopher connection for data */
			if (g_gopher.receiving) {
				if (gopher_idle(&g_gopher)) {
					GrafPtr save;

					GetPort(&save);
					SetPort(g_window);
					draw_page();
					SetPort(save);
				}
				if (!g_gopher.receiving) {
					char uri[300];
					GrafPtr save;

					g_app_state = APP_STATE_IDLE;
					set_wtitlef(g_window,
					    "Geomys - %s",
					    g_gopher.cur_host);

					/* Update address bar and status */
					gopher_build_uri(uri, sizeof(uri),
					    g_gopher.cur_host,
					    g_gopher.cur_port,
					    g_gopher.cur_type,
					    g_gopher.cur_selector);
					browser_set_url(uri);

					if (g_gopher.page_type ==
					    PAGE_DIRECTORY)
						snprintf(uri, sizeof(uri),
						    "Done \xD0 %d items",
						    g_gopher.item_count);
					else
						snprintf(uri, sizeof(uri),
						    "Done \xD0 %ld bytes",
						    g_gopher.text_len);
					browser_set_status(uri);

					GetPort(&save);
					SetPort(g_window);
					browser_draw_status(g_window);
					browser_draw(g_window);
					SetPort(save);
				}
			}

			/* Address bar cursor blink */
			if (!g_suspended) {
				GrafPtr save;

				GetPort(&save);
				SetPort(g_window);
				browser_idle();
				SetPort(save);
			}
			break;
		case keyDown:
			handle_key_down(&event);
			break;
		case autoKey:
			/* Text repeat in address bar */
			if (!(event.modifiers & cmdKey)) {
				GrafPtr save;

				GetPort(&save);
				SetPort(g_window);
				browser_key(g_window, &event);
				SetPort(save);
			}
			break;
		case mouseDown:
			handle_mouse_down(&event);
			break;
		case updateEvt:
			handle_update(&event);
			break;
		case activateEvt:
			handle_activate(&event);
			break;
		case app4Evt:
			if (HiWord(event.message) & (1 << 8)) {
				if (event.message & 1)
					g_suspended = false;
				else
					g_suspended = true;
			}
			break;
		case kHighLevelEvent:
			AEProcessAppleEvent(&event);
			break;
		}
	}

	browser_cleanup();
	gopher_cleanup(&g_gopher);

	if (g_window)
		DisposeWindow(g_window);

	ExitToShell();
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

	/* Pass to address bar */
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_key(g_window, event);
		SetPort(save);
	}
}

/*
 * Navigate to a gopher:// URL string
 */
void
do_navigate_url(const char *url)
{
	char host[64];
	short port;
	char type;
	char selector[256];
	char uri[300];

	if (!gopher_parse_uri(url, host, sizeof(host),
	    &port, &type, selector, sizeof(selector)))
		return;

	g_app_state = APP_STATE_LOADING;

	snprintf(uri, sizeof(uri), "Loading %.50s\311", host);
	browser_set_status(uri);
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	}

	gopher_navigate(&g_gopher, host, port, type, selector);
}

/*
 * Open URL dialog (Cmd-L)
 */
void
do_open_url_dialog(void)
{
	DialogPtr dlg;
	short item;
	char cur_url[300];
	Str255 pstr;
	short item_type;
	Handle item_h;
	Rect item_rect;

	dlg = GetNewDialog(DLOG_OPEN_URL_ID, 0L, (WindowPtr)-1L);
	if (!dlg)
		return;

	/* Pre-fill with current URL */
	browser_get_url(cur_url, sizeof(cur_url));
	if (cur_url[0]) {
		c2pstr(pstr, cur_url);
		GetDialogItem(dlg, 4, &item_type, &item_h, &item_rect);
		SetDialogItemText(item_h, pstr);
		SelectDialogItemText(dlg, 4, 0, 32767);
	}

	setup_default_button_outline(dlg, 5);

	ModalDialog((ModalFilterUPP)std_dlg_filter, &item);

	if (item == 1) {
		char url[300];

		/* Get URL from field */
		GetDialogItem(dlg, 4, &item_type, &item_h, &item_rect);
		GetDialogItemText(item_h, pstr);
		{
			short len = pstr[0];
			if (len >= (short)sizeof(url))
				len = sizeof(url) - 1;
			memcpy(url, pstr + 1, len);
			url[len] = '\0';
		}

		DisposeDialog(dlg);

		if (url[0])
			do_navigate_url(url);
	} else {
		DisposeDialog(dlg);
	}
}

static void
handle_nav_button(short btn_id)
{
	switch (btn_id) {
	case NAV_BTN_BACK:
		/* Phase 6 */
		break;
	case NAV_BTN_FORWARD:
		/* Phase 6 */
		break;
	case NAV_BTN_REFRESH: {
		/* Re-fetch current page */
		char url[300];

		gopher_build_uri(url, sizeof(url),
		    g_gopher.cur_host, g_gopher.cur_port,
		    g_gopher.cur_type, g_gopher.cur_selector);
		do_navigate_url(url);
		break;
	}
	case NAV_BTN_HOME:
		do_navigate_url(DEFAULT_HOME_URL);
		break;
	}
}

/*
 * draw_page - render current Gopher page content in the content area.
 */
static void
draw_page(void)
{
	Rect r;
	short i, y;
	Str255 ps;
	short len;
	RgnHandle save_clip;

	browser_get_content_rect(g_window, &r);

	/* Clip to content area */
	save_clip = NewRgn();
	GetClip(save_clip);
	ClipRect(&r);
	EraseRect(&r);

	if (g_gopher.page_type == PAGE_DIRECTORY) {
		TextFont(4);  /* Monaco — monospaced for alignment */
		TextSize(9);

		y = r.top + 11;
		for (i = 0; i < g_gopher.item_count; i++) {
			GopherItem *item = &g_gopher.items[i];
			char line[100];
			const char *label;

			if (y > r.bottom - 4)
				break;

			label = gopher_type_label(item->type);

			if (item->type == GOPHER_INFO)
				snprintf(line, sizeof(line),
				    "      %s", item->display);
			else
				snprintf(line, sizeof(line),
				    " %s  %s", label, item->display);

			len = strlen(line);
			if (len > 255) len = 255;
			ps[0] = len;
			memcpy(ps + 1, line, len);

			MoveTo(r.left + 4, y);
			DrawString(ps);
			y += 11;
		}
	} else if (g_gopher.page_type == PAGE_TEXT) {
		TextFont(4);  /* Monaco */
		TextSize(9);

		{
			const char *p = g_gopher.text_buf;
			const char *end = p + g_gopher.text_len;

			y = r.top + 11;
			while (p < end && y < r.bottom - 4) {
				const char *line_start = p;
				short line_len;

				while (p < end && *p != '\r')
					p++;
				line_len = p - line_start;
				if (p < end)
					p++;

				if (line_len > 80)
					line_len = 80;

				MoveTo(r.left + 4, y);
				DrawText((Ptr)line_start, 0, line_len);
				y += 11;
			}
		}
	}

	SetClip(save_clip);
	DisposeRgn(save_clip);
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
		if (TrackGoAway(win, event->where))
			g_running = false;
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
			SizeWindow(win, LoWord(new_size),
			    HiWord(new_size), true);
			{
				GrafPtr save;

				GetPort(&save);
				SetPort(win);
				InvalRect(&win->portRect);
				SetPort(save);
			}
		}
		break;
	}
	case inContent:
		if (win != FrontWindow()) {
			SelectWindow(win);
		} else if (win == g_window) {
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
			} else if (click_result == -1) {
				/* Content area click */
				Rect content_r;

				browser_get_content_rect(win,
				    &content_r);

				if (PtInRect(local_pt, &content_r) &&
				    g_gopher.page_type ==
				    PAGE_DIRECTORY &&
				    g_gopher.item_count > 0) {
					short clicked_item;

					clicked_item = (local_pt.v -
					    content_r.top) / 11;
					if (clicked_item >= 0 &&
					    clicked_item <
					    g_gopher.item_count) {
						GopherItem *item =
						    &g_gopher.items[
						    clicked_item];
						if (gopher_type_navigable(
						    item->type)) {
							Rect h_r;

							SetRect(&h_r,
							    content_r
							    .left,
							    content_r
							    .top +
							    clicked_item
							    * 11,
							    content_r
							    .right,
							    content_r
							    .top +
							    clicked_item
							    * 11 + 11);
							InvertRect(
							    &h_r);

							g_app_state =
							    APP_STATE_LOADING;
							gopher_navigate(
							    &g_gopher,
							    item->host,
							    item->port,
							    item->type,
							    item
							    ->selector);
						}
					}
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

	win = (WindowPtr)event->message;

	GetPort(&old_port);
	SetPort(win);
	BeginUpdate(win);

	if (win == g_window) {
		browser_draw(win);
		draw_page();
		DrawGrowIcon(win);
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

	win = (WindowPtr)event->message;

	if (win == g_window) {
		if (event->modifiers & activeFlag)
			browser_activate(true);
		else
			browser_activate(false);
	}
}
