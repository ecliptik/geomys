/*
 * offscreen.h - Offscreen double-buffer rendering
 *
 * Eliminates flicker by redirecting QuickDraw drawing to an
 * invisible bitmap, then blitting the result to screen in one
 * CopyBits call.
 */

#ifndef OFFSCREEN_H
#define OFFSCREEN_H

#ifdef GEOMYS_OFFSCREEN

#include <Quickdraw.h>
#include <Windows.h>

/* Allocate offscreen buffer sized to window's portRect.
 * Call once at startup after window creation. Returns 1 on
 * success, 0 if allocation failed (caller falls back to
 * direct drawing). */
short offscreen_init(WindowPtr win);

/* Free offscreen buffer */
void offscreen_cleanup(void);

/* Begin offscreen rendering — redirects QuickDraw to buffer.
 * All drawing between begin/end goes to the invisible bitmap. */
void offscreen_begin(WindowPtr win);

/* End offscreen rendering — CopyBits the given rect from
 * offscreen buffer to screen, then restores portBits.
 * Pass the content area rect to blit only the changed region. */
void offscreen_end(WindowPtr win, const Rect *blit_rect);

/* Check if offscreen is available and allocated */
short offscreen_is_ready(void);

/* Reallocate offscreen buffer if window grew beyond current
 * buffer size. No-op if buffer is already large enough. */
void offscreen_resize(WindowPtr win);

#endif /* GEOMYS_OFFSCREEN */
#endif /* OFFSCREEN_H */
