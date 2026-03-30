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

/* Initialize the shared offscreen buffer sized to window's portRect.
 * Idempotent: if a buffer already exists, grows it to cover the
 * window if needed and returns 1. Returns 1 on success, 0 if
 * allocation failed (caller falls back to direct drawing). */
short offscreen_init(WindowPtr win);

/* Free offscreen buffer */
void offscreen_cleanup(void);

/* Begin offscreen rendering — redirects QuickDraw to buffer.
 * All drawing between begin/end goes to the invisible bitmap.
 * Returns 1 on success, 0 if offscreen could not be activated
 * (caller should fall back to direct drawing). */
short offscreen_begin(WindowPtr win);

/* End offscreen rendering — CopyBits the given rect from
 * offscreen buffer to screen, then restores portBits.
 * Pass the content area rect to blit only the changed region. */
void offscreen_end(WindowPtr win, const Rect *blit_rect);

/* Check if offscreen is available and allocated */
short offscreen_is_ready(void);

/* Check if color GWorld path is active (display depth > 1).
 * Use this instead of g_has_color_qd when deciding between
 * color (RGBBackColor+EraseRect) and mono (PaintRect+srcBic)
 * drawing — Color QD may be present on a 1-bit display. */
short offscreen_is_color(void);

/* Grow the shared offscreen buffer if window exceeds current
 * buffer size. No-op if buffer is already large enough. The
 * buffer never shrinks — freed once at app exit. */
void offscreen_resize(WindowPtr win);

#else
/* No offscreen: color path never active */
#define offscreen_is_color() 0
#endif /* GEOMYS_OFFSCREEN */
#endif /* OFFSCREEN_H */
