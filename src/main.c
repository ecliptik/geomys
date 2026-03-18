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
/* do_navigate_url and do_open_url_dialog declared in main.h */
static void handle_nav_button(short btn_id);
static void update_nav_buttons(void);
static void navigate_history_entry(const HistoryEntry *e, short direction);

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

	/* Initialize browser chrome and content area */
	{
		GrafPtr save;
		GetPort(&save);
		SetPort(g_window);
		browser_init(g_window);
		content_init(g_window);
		SetPort(save);
	}

	update_menus();

	/* Initialize Gopher engine and history */
	gopher_init(&g_gopher);
	content_set_page(&g_gopher);
	history_init();
	do_navigate_url(DEFAULT_HOME_URL);

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
					content_draw(g_window);
					content_update_scroll(
					    g_window);
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

					/* Update nav button states */
					update_nav_buttons();

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

	content_cleanup();
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
		/* Cmd-[ = back, Cmd-] = forward */
		if (key == '[') {
			if (history_can_back())
				navigate_history_entry(
				    history_back(), -1);
			return;
		}
		if (key == ']') {
			if (history_can_forward())
				navigate_history_entry(
				    history_forward(), 1);
			return;
		}
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
	content_scroll_to_top();

	snprintf(uri, sizeof(uri), "Loading %.50s\311", host);
	browser_set_status(uri);
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	}

	if (!gopher_navigate(&g_gopher, host, port, type, selector)) {
		GrafPtr save;

		/* Navigate failed — old page preserved, force redraw */
		g_app_state = APP_STATE_IDLE;
		browser_set_status("Connection failed");
		GetPort(&save);
		SetPort(g_window);
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	} else {
		/* Success — push to history */
		history_push(host, port, type, selector, host);
		update_nav_buttons();
	}
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

	/* Loop until Connect or Cancel button clicked */
	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter, &item);
	} while (item != 1 && item != 2);

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

	dlg = GetNewDialog(DLOG_SEARCH_ID, 0L, (WindowPtr)-1L);
	if (!dlg) {
		browser_activate(true);
		return;
	}

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

		if (query[0]) {
			/* Append query to selector with tab */
			snprintf(full_sel, sizeof(full_sel),
			    "%s\t%s", selector, query);
			g_app_state = APP_STATE_LOADING;
			content_scroll_to_top();
			gopher_navigate(&g_gopher, host, port,
			    GOPHER_DIRECTORY, full_sel);
		}
	} else {
		DisposeDialog(dlg);
		browser_activate(true);
	}
}

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
	case GOPHER_HTML: {
		/* Show the URL if display starts with "URL:" */
		const char *url = display;
		if (strncmp(display, "URL:", 4) == 0)
			url = display + 4;
		snprintf(msg, sizeof(msg),
		    "External link: %.120s\n"
		    "Copy this URL to a web browser.", url);
		break;
	}
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

static void
update_nav_buttons(void)
{
	GrafPtr save;

	browser_set_button_state(NAV_BTN_BACK,
	    history_can_back() ? BTN_ENABLED : BTN_DISABLED);
	browser_set_button_state(NAV_BTN_FORWARD,
	    history_can_forward() ? BTN_ENABLED : BTN_DISABLED);

	GetPort(&save);
	SetPort(g_window);
	browser_draw(g_window);
	SetPort(save);
}

/*
 * Navigate to a history entry.
 * direction: -1 = came from back, +1 = came from forward
 */
static void
navigate_history_entry(const HistoryEntry *e, short direction)
{
	char uri[300];

	if (!e)
		return;

	g_app_state = APP_STATE_LOADING;
	content_scroll_to_top();

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

	/* Navigate without pushing to history */
	if (!gopher_navigate(&g_gopher, e->host, e->port,
	    e->type, e->selector)) {
		GrafPtr save;

		/* Undo the history move */
		if (direction < 0)
			history_undo_back();
		else
			history_undo_forward();

		g_app_state = APP_STATE_IDLE;
		browser_set_status("Connection failed");
		GetPort(&save);
		SetPort(g_window);
		content_draw(g_window);
		content_update_scroll(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	}

	update_nav_buttons();
}

static void
handle_nav_button(short btn_id)
{
	switch (btn_id) {
	case NAV_BTN_BACK:
		navigate_history_entry(history_back(), -1);
		break;
	case NAV_BTN_FORWARD:
		navigate_history_entry(history_forward(), 1);
		break;
	case NAV_BTN_REFRESH: {
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
			GrafPtr save;

			SizeWindow(win, LoWord(new_size),
			    HiWord(new_size), true);
			GetPort(&save);
			SetPort(win);
			content_resize(win);
			InvalRect(&win->portRect);
			SetPort(save);
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
				/* Check scroll bar first */
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

	win = (WindowPtr)event->message;

	GetPort(&old_port);
	SetPort(win);
	BeginUpdate(win);

	if (win == g_window) {
		browser_draw(win);
		content_draw(win);
		content_update_scroll(win);
		DrawControls(win);

		/* Draw grow icon clipped to bottom-right corner */
		{
			Rect clip_r;
			RgnHandle save_clip;

			save_clip = NewRgn();
			GetClip(save_clip);
			SetRect(&clip_r,
			    win->portRect.right - SCROLLBAR_WIDTH,
			    win->portRect.bottom - STATUS_BAR_HEIGHT,
			    win->portRect.right,
			    win->portRect.bottom);
			ClipRect(&clip_r);
			DrawGrowIcon(win);
			SetClip(save_clip);
			DisposeRgn(save_clip);
		}
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
