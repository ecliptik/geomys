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

/* Globals */
Boolean g_running = true;
Boolean g_suspended = false;
WindowPtr g_window = 0L;
short g_app_state = APP_STATE_IDLE;

/* Forward declarations */
static void init_toolbox(void);
static void init_apple_events(void);
static void create_window(void);
static void main_event_loop(void);
static void handle_mouse_down(EventRecord *event);
static void handle_update(EventRecord *event);
static void handle_activate(EventRecord *event);

/*
 * Apple Events handlers for System 7 compatibility.
 * Required by Finder to properly launch/quit the app.
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
	SetRect(&bounds, 2, 22, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2);
	g_window = NewWindow(0L, &bounds, "\pGeomys", true,
	    noGrowDocProc, (WindowPtr)-1L, true, 0L);
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
			wait_ticks = 1L;
		else
			wait_ticks = 10L;

		WaitNextEvent(everyEvent, &event, wait_ticks, 0L);

		switch (event.what) {
		case nullEvent:
			/* Idle processing — network polling will go here */
			break;
		case keyDown:
			if (event.modifiers & cmdKey) {
				update_menus();
				handle_menu(MenuKey(
				    event.message & charCodeMask));
			}
			break;
		case autoKey:
			/* Don't dispatch menu commands on autoKey —
			 * only handle repeat for text input (Phase 3+) */
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

	if (g_window)
		DisposeWindow(g_window);

	ExitToShell();
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
		if (win != FrontWindow())
			SelectWindow(win);
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

	/* Draw content area — placeholder for Phase 1 */
	EraseRect(&win->portRect);

	EndUpdate(win);
	SetPort(old_port);
}

static void
handle_activate(EventRecord *event)
{
	WindowPtr win;

	win = (WindowPtr)event->message;

	if (event->modifiers & activeFlag) {
		/* Window activated */
	} else {
		/* Window deactivated */
	}

	(void)win;
}
