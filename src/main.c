/*
 * main.c - Geomys: Gopher browser for classic Macintosh
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
static void handle_update(EventRecord *event);
static void handle_activate(EventRecord *event);
static void draw_page(void);

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
	update_menus();

	/* Initialize Gopher engine and navigate to home page */
	gopher_init(&g_gopher);
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

	/* Full screen minus menu bar, inset slightly */
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
					/* New data — redraw */
					GrafPtr save;

					GetPort(&save);
					SetPort(g_window);
					draw_page();
					SetPort(save);
				}
				if (!g_gopher.receiving) {
					g_app_state = APP_STATE_IDLE;
					/* Update title with host */
					set_wtitlef(g_window,
					    "Geomys - %s",
					    g_gopher.cur_host);
				}
			}
			break;
		case keyDown:
			if (event.modifiers & cmdKey) {
				update_menus();
				handle_menu(MenuKey(
				    event.message & charCodeMask));
			}
			break;
		case autoKey:
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

	gopher_cleanup(&g_gopher);

	if (g_window)
		DisposeWindow(g_window);

	ExitToShell();
}

/*
 * draw_page - render current Gopher page content in the window.
 * Basic rendering for Phase 2 — just text lines.
 */
static void
draw_page(void)
{
	Rect r;
	short i, y;
	Str255 ps;
	short len;

	r = g_window->portRect;
	EraseRect(&r);

	if (g_gopher.page_type == PAGE_DIRECTORY) {
		TextFont(4);  /* Monaco — monospaced for alignment */
		TextSize(9);

		y = 12;
		for (i = 0; i < g_gopher.item_count; i++) {
			GopherItem *item = &g_gopher.items[i];
			char line[100];
			const char *label;

			if (y > r.bottom - 4)
				break;  /* off screen */

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

			MoveTo(4, y);
			DrawString(ps);
			y += 11;
		}
	} else if (g_gopher.page_type == PAGE_TEXT) {
		TextFont(4);  /* Monaco */
		TextSize(9);

		/* Simple text rendering — draw lines */
		{
			const char *p = g_gopher.text_buf;
			const char *end = p + g_gopher.text_len;

			y = 12;
			while (p < end && y < r.bottom - 4) {
				const char *line_start = p;
				short line_len;

				/* Find end of line */
				while (p < end && *p != '\r')
					p++;
				line_len = p - line_start;
				if (p < end)
					p++;  /* skip CR */

				if (line_len > 80)
					line_len = 80;

				MoveTo(4, y);
				DrawText((Ptr)line_start, 0, line_len);
				y += 11;
			}
		}
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
			g_running = false;
		}
		break;
	case inContent:
		if (win != FrontWindow()) {
			SelectWindow(win);
		} else if (win == g_window) {
			/* Handle click on Gopher items */
			Point local_pt;
			GrafPtr save;
			short clicked_item;

			GetPort(&save);
			SetPort(win);
			local_pt = event->where;
			GlobalToLocal(&local_pt);

			/* Calculate which item was clicked */
			if (g_gopher.page_type == PAGE_DIRECTORY &&
			    g_gopher.item_count > 0) {
				clicked_item =
				    (local_pt.v - 2) / 11;
				if (clicked_item >= 0 &&
				    clicked_item <
				    g_gopher.item_count) {
					GopherItem *item =
					    &g_gopher.items[
					    clicked_item];
					if (gopher_type_navigable(
					    item->type)) {
						Rect hilite_r;

						/* Inverse highlight
						 * feedback */
						SetRect(&hilite_r,
						    0,
						    clicked_item * 11
						    + 2,
						    win->portRect
						    .right,
						    clicked_item * 11
						    + 13);
						InvertRect(
						    &hilite_r);

						g_app_state =
						    APP_STATE_LOADING;
						gopher_navigate(
						    &g_gopher,
						    item->host,
						    item->port,
						    item->type,
						    item->selector);
					}
				}
			}

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

	if (win == g_window)
		draw_page();
	else
		EraseRect(&win->portRect);

	EndUpdate(win);
	SetPort(old_port);
}

static void
handle_activate(EventRecord *event)
{
	(void)event;
}
