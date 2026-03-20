/*
 * session.h - Multi-window session management for Geomys
 *
 * Each BrowserSession holds all per-window state: window, gopher engine,
 * history, browser chrome, and content area. For single-window builds
 * (GEOMYS_MAX_WINDOWS == 1), active_session is a compile-time macro
 * with zero overhead.
 */

#ifndef SESSION_H
#define SESSION_H

#include <Windows.h>
#include <TextEdit.h>

#include "gopher.h"
#include "history.h"
#include "browser.h"
#ifdef GEOMYS_CLIPBOARD
#include "content.h"
#endif

#ifndef GEOMYS_MAX_WINDOWS
#define GEOMYS_MAX_WINDOWS 1
#endif

/* Window cascade offset for multi-window */
#define CASCADE_OFFSET  20

typedef struct BrowserSession {
	/* Window */
	WindowPtr       window;
	ControlHandle   scrollbar;

	/* Gopher engine state */
	GopherState     gopher;

	/* Navigation history */
	HistoryEntry    history[HISTORY_MAX];
	short           history_count;
	short           history_pos;

	/* Browser chrome state */
	TEHandle        addr_te;
	char            status[80];
	short           btn_state[NAV_BTN_COUNT];
	Rect            btn_rects[NAV_BTN_COUNT];
	Rect            addr_rect;
	short           focus;

	/* Content area state */
	short           scroll_pos;
	short           hover_row;
	short           row_height;
	short           font_id;
	short           font_size;

#ifdef GEOMYS_CLIPBOARD
	/* Selection state */
	Selection       sel;
	short           win_active;
#endif

	/* Application state */
	short           app_state;
	short           pending_scroll;

	/* Session slot index (0..GEOMYS_MAX_WINDOWS-1) */
	short           id;
} BrowserSession;

/* Session lifecycle */
BrowserSession *session_new(void);
void session_destroy(BrowserSession *s);
void session_destroy_and_fixup(BrowserSession *s);

/* Lookup */
BrowserSession *session_from_window(WindowPtr win);
short session_count(void);
BrowserSession *session_get(short index);

/* Active session and state save/restore */
#if GEOMYS_MAX_WINDOWS > 1
extern BrowserSession *active_session;
void session_save_state(BrowserSession *s);
void session_load_state(BrowserSession *s);
void session_switch_to(BrowserSession *s);

/* Lightweight save/restore: only browser + content + gopher state,
 * skips history (~6.6 KB). For background polling that doesn't
 * touch history. */
void session_save_gopher(BrowserSession *s);
void session_load_gopher(BrowserSession *s);
void session_switch_to_light(BrowserSession *s);
#else
/* Zero overhead: static session, no save/restore needed */
extern BrowserSession g_single_session;
#define active_session  (&g_single_session)
#define session_save_state(s)  ((void)0)
#define session_load_state(s)  ((void)0)
#define session_switch_to(s)   ((void)0)
#define session_save_gopher(s)      ((void)0)
#define session_load_gopher(s)      ((void)0)
#define session_switch_to_light(s)  ((void)0)
#endif

#endif /* SESSION_H */
