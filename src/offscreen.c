/*
 * offscreen.c - Offscreen double-buffer rendering
 *
 * Allocates a 1-bit deep offscreen BitMap matching the window's
 * content area. Drawing is redirected via SetPortBits, then
 * CopyBits blits the result to screen — eliminating all flicker
 * from erase-then-draw sequences.
 *
 * Memory: ~22KB for 512x342 (64 rowBytes * 342 rows).
 * Reference: Flynn terminal_ui.c SetPortBits pattern.
 */

#ifdef GEOMYS_OFFSCREEN

#include <Quickdraw.h>
#include <Windows.h>
#include <Memory.h>
#include <string.h>

#include "offscreen.h"

static BitMap  g_offscreen;      /* offscreen bitmap descriptor */
static Ptr     g_offscreen_bits; /* pixel data (NewPtr) */
static BitMap  g_saved_bits;     /* saved real portBits */
static short   g_ready;          /* 1 if buffer allocated */
static short   g_active;         /* 1 if between begin/end */

short
offscreen_init(WindowPtr win)
{
	short pixel_w, pixel_h, rb;
	long size;

	if (g_offscreen_bits)
		return 1;  /* already allocated */

	pixel_w = win->portRect.right - win->portRect.left;
	pixel_h = win->portRect.bottom - win->portRect.top;

	/* rowBytes must be even and >= (pixel_w + 7) / 8 */
	rb = ((pixel_w + 15) / 16) * 2;
	size = (long)rb * pixel_h;

	g_offscreen_bits = NewPtr(size);
	if (!g_offscreen_bits) {
		g_ready = 0;
		return 0;
	}

	memset(g_offscreen_bits, 0xFF, size);  /* white fill */

	g_offscreen.baseAddr = g_offscreen_bits;
	g_offscreen.rowBytes = rb;
	g_offscreen.bounds = win->portRect;

	g_ready = 1;
	g_active = 0;
	return 1;
}

void
offscreen_cleanup(void)
{
	if (g_offscreen_bits) {
		DisposePtr(g_offscreen_bits);
		g_offscreen_bits = 0L;
	}
	g_ready = 0;
	g_active = 0;
}

void
offscreen_begin(WindowPtr win)
{
	if (!g_ready || g_active)
		return;

	/* Save the real screen bits */
	g_saved_bits = win->portBits;

	/* Redirect QuickDraw to offscreen */
	SetPortBits(&g_offscreen);
	g_active = 1;
}

void
offscreen_end(WindowPtr win, const Rect *blit_rect)
{
	if (!g_active)
		return;

	/* Restore real portBits */
	SetPortBits(&g_saved_bits);
	g_active = 0;

	/* Blit offscreen to screen */
	CopyBits(&g_offscreen, &win->portBits,
	    blit_rect, blit_rect,
	    srcCopy, 0L);
}

short
offscreen_is_ready(void)
{
	return g_ready;
}

#endif /* GEOMYS_OFFSCREEN */
