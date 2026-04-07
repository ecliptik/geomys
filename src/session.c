/*
 * session.c - Multi-window session management for Geomys
 */

#include <Memory.h>
#include <Windows.h>
#include <string.h>

#include "session.h"
#include "main.h"
#include "history.h"
#include "browser.h"
#include "content.h"
#include "gopher.h"
#include "connection.h"
#include "drag.h"
#ifdef GEOMYS_THEMES
#include "theme.h"
#endif

#if GEOMYS_MAX_WINDOWS == 1

/*
 * Single-session fast path: static session, no dynamic allocation.
 */
BrowserSession g_single_session;
static short g_num_sessions = 0;

BrowserSession *
session_new(void)
{
	if (g_num_sessions > 0)
		return 0L;

	memset(&g_single_session, 0, sizeof(g_single_session));
	g_single_session.id = 0;
	g_single_session.pending_scroll = -1;
	g_num_sessions = 1;
	return &g_single_session;
}

void
session_destroy(BrowserSession *s)
{
	if (!s)
		return;

	/* Close active connection */
	if (s->gopher.conn.state != CONN_STATE_IDLE)
		conn_close(&s->gopher.conn);

	/* Cleanup gopher engine (frees items, text_buf, text_lines) */
	gopher_cleanup(&s->gopher);

	/* Dispose TEHandle (address bar) */
	if (s->addr_te) {
		TEDispose(s->addr_te);
		s->addr_te = 0L;
	}

	/* Remove drag handlers before window disposal */
#ifdef GEOMYS_DRAG
	drag_remove_handlers(s->window);
#endif

	/* DisposeWindow auto-disposes controls (scrollbar) */
	if (s->window) {
		DisposeWindow(s->window);
		s->window = 0L;
	}

	s->scrollbar = 0L;
	g_num_sessions = 0;
}

void
session_destroy_and_fixup(BrowserSession *s)
{
	session_destroy(s);

	/* Single-session: no fixup needed, app stays running */
}

BrowserSession *
session_from_window(WindowPtr win)
{
	if (win && win == g_single_session.window)
		return &g_single_session;
	return 0L;
}

short
session_count(void)
{
	return g_num_sessions;
}

BrowserSession *
session_get(short index)
{
	if (index == 0 && g_num_sessions > 0)
		return &g_single_session;
	return 0L;
}

#else /* GEOMYS_MAX_WINDOWS > 1 */

/*
 * Multi-session: dynamic allocation, pointer array.
 */
static BrowserSession *sessions[GEOMYS_MAX_WINDOWS];
static short g_num_sessions = 0;
BrowserSession *active_session = 0L;

BrowserSession *
session_new(void)
{
	BrowserSession *s;
	short i, slot;

	/* Find empty slot */
	slot = -1;
	for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
		if (sessions[i] == 0L) {
			slot = i;
			break;
		}
	}
	if (slot < 0)
		return 0L;

	/* Allocate */
	s = (BrowserSession *)NewPtr(sizeof(BrowserSession));
	if (s == 0L)
		return 0L;

	memset(s, 0, sizeof(BrowserSession));
	s->id = slot;
	s->pending_scroll = -1;

	sessions[slot] = s;
	g_num_sessions++;
	return s;
}

void
session_destroy(BrowserSession *s)
{
	if (s == 0L)
		return;

	/* Close active connection */
	if (s->gopher.conn.state != CONN_STATE_IDLE)
		conn_close(&s->gopher.conn);

	/* Cleanup gopher engine (frees items, text_buf, text_lines) */
	gopher_cleanup(&s->gopher);

	/* Dispose TEHandle (address bar) */
	if (s->addr_te) {
		TEDispose(s->addr_te);
		s->addr_te = 0L;
	}

	/* Remove drag handlers before window disposal */
#ifdef GEOMYS_DRAG
	drag_remove_handlers(s->window);
#endif

	/* DisposeWindow auto-disposes controls (scrollbar) */
	if (s->window) {
		DisposeWindow(s->window);
		s->window = 0L;
	}

	if (s->id >= 0 && s->id < GEOMYS_MAX_WINDOWS)
		sessions[s->id] = 0L;
	g_num_sessions--;

	DisposePtr((Ptr)s);
}

void
session_destroy_and_fixup(BrowserSession *s)
{
	short was_active = (s == active_session);

	if (was_active) {
		/* Clear slot BEFORE destroy so session_from_window
		 * won't find it during DisposeWindow event processing */
		if (s->id >= 0 && s->id < GEOMYS_MAX_WINDOWS)
			sessions[s->id] = 0L;

		/* If other windows remain, switch to one BEFORE
		 * destroying — this ensures module statics point
		 * to valid data during DisposeWindow events */
		if (g_num_sessions > 1) {
			WindowPtr front;
			BrowserSession *next = 0L;

			/* Find another session to switch to */
			front = FrontWindow();
			if (front && front != s->window)
				next = session_from_window(front);
			if (!next) {
				short i;

				for (i = 0; i < GEOMYS_MAX_WINDOWS;
				    i++) {
					if (sessions[i] &&
					    sessions[i] != s) {
						next = sessions[i];
						break;
					}
				}
			}
			if (next) {
				active_session = next;
				session_load_state(next);
			} else {
				active_session = 0L;
			}
		} else {
			active_session = 0L;
		}

		/* Restore slot for destroy cleanup */
		if (s->id >= 0 && s->id < GEOMYS_MAX_WINDOWS)
			sessions[s->id] = s;
	}

	session_destroy(s);

	if (g_num_sessions == 0) {
		/* Last window closed — app stays running */
		return;
	}
}

BrowserSession *
session_from_window(WindowPtr win)
{
	short i;

	if (win == 0L)
		return 0L;

	/* Fast path: check active session first (most common case) */
	if (active_session && active_session->window == win)
		return active_session;

	for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
		if (sessions[i] && sessions[i]->window == win)
			return sessions[i];
	}
	return 0L;
}

short
session_count(void)
{
	return g_num_sessions;
}

BrowserSession *
session_get(short index)
{
	if (index < 0 || index >= GEOMYS_MAX_WINDOWS)
		return 0L;
	return sessions[index];
}

void
session_save_state(BrowserSession *s)
{
	if (!s)
		return;
#ifdef GEOMYS_THEMES
	s->theme_id = theme_get();
#endif
	history_save_state(s);
	browser_save_state(s);
	content_save_state(s);
}

void
session_load_state(BrowserSession *s)
{
	if (!s)
		return;
#ifdef GEOMYS_THEMES
	if (s->theme_id != theme_get()) {
		theme_set(s->theme_id);
		theme_reset_cache();
		content_invalidate_shadow();
		content_mark_all_dirty();
	}
#endif
	history_load_state(s);
	browser_load_state(s);
	content_load_state(s);
}

/*
 * session_switch_to - Switch active session with state save/restore.
 * No-op if target is already active.
 */
void
session_switch_to(BrowserSession *s)
{
	if (s == active_session)
		return;
	if (active_session)
		session_save_state(active_session);
	active_session = s;
	if (s)
		session_load_state(s);
}

/*
 * Lightweight save/restore: skip history (~6.6 KB memcpy).
 * Used during background polling where history is untouched.
 */
void
session_save_gopher(BrowserSession *s)
{
	if (!s)
		return;
#ifdef GEOMYS_THEMES
	s->theme_id = theme_get();
#endif
	browser_save_state(s);
	content_save_state(s);
}

void
session_load_gopher(BrowserSession *s)
{
	if (!s)
		return;
#ifdef GEOMYS_THEMES
	if (s->theme_id != theme_get()) {
		theme_set(s->theme_id);
		theme_reset_cache();
		content_invalidate_shadow();
		content_mark_all_dirty();
	}
#endif
	browser_load_state(s);
	content_load_state(s);
}

/*
 * session_switch_to_light - Lightweight session switch for
 * background polling. Skips history save/restore.
 */
void
session_switch_to_light(BrowserSession *s)
{
	if (s == active_session)
		return;
	if (active_session)
		session_save_gopher(active_session);
	active_session = s;
	if (s)
		session_load_gopher(s);
}

#endif /* GEOMYS_MAX_WINDOWS */
