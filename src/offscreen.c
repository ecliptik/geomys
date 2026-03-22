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

#ifdef GEOMYS_COLOR
	{
		extern unsigned char g_has_color_qd;
		if (g_has_color_qd) {
			GDHandle gd = GetMainDevice();
			if (gd && (*(*gd)->gdPMap)->pixelSize > 1) {
				/* Actual color display — skip offscreen, draw direct.
				 * Mac II+ is fast enough for row-by-row drawing. */
				g_ready = 0;
				return 0;
			}
			/* Monochrome display with Color QD (e.g. System 7)
			 * — use offscreen buffer for flicker-free drawing. */
		}
	}
#endif

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
	short win_w, win_h, buf_w, buf_h;

	if (!g_ready || g_active)
		return;

	/* Validate buffer is large enough for this window.
	 * If the window was resized larger than the buffer,
	 * skip offscreen to avoid memory corruption. */
	win_w = win->portRect.right - win->portRect.left;
	win_h = win->portRect.bottom - win->portRect.top;
	buf_w = (g_offscreen.rowBytes * 8);
	buf_h = g_offscreen.bounds.bottom - g_offscreen.bounds.top;
	if (win_w > buf_w || win_h > buf_h)
		return;

	/* Save the real screen bits */
	g_saved_bits = win->portBits;

	/* Update offscreen bounds to match current window's
	 * coordinate system — critical for multi-window support
	 * where each window may have different portRect */
	g_offscreen.bounds = win->portRect;

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

void
offscreen_resize(WindowPtr win)
{
	short pixel_w, pixel_h, rb;
	long size;
	Ptr new_bits;

	if (!g_ready)
		return;

	pixel_w = win->portRect.right - win->portRect.left;
	pixel_h = win->portRect.bottom - win->portRect.top;

	/* Check if current buffer is large enough */
	rb = ((pixel_w + 15) / 16) * 2;
	if (rb <= g_offscreen.rowBytes &&
	    pixel_h <= (g_offscreen.bounds.bottom -
	    g_offscreen.bounds.top))
		return;  /* buffer is big enough */

	/* Reallocate with new size */
	if (rb < g_offscreen.rowBytes)
		rb = g_offscreen.rowBytes;
	size = (long)rb * pixel_h;

	new_bits = NewPtr(size);
	if (!new_bits)
		return;  /* keep old buffer */

	memset(new_bits, 0xFF, size);  /* white fill */

	/* Free old buffer and install new one */
	DisposePtr(g_offscreen_bits);
	g_offscreen_bits = new_bits;
	g_offscreen.baseAddr = new_bits;
	g_offscreen.rowBytes = rb;
	g_offscreen.bounds = win->portRect;
}

#endif /* GEOMYS_OFFSCREEN */
