/*
 * offscreen.c - Shared offscreen double-buffer rendering
 *
 * A single offscreen buffer is shared across all browser windows.
 * Cooperative multitasking guarantees only one window draws at a
 * time, so one buffer is sufficient. The buffer grows to fit the
 * largest window but never shrinks (freed once at app exit).
 *
 * Mono path: 1-bit BitMap via SetPortBits (~22KB for 512x342).
 * Color path: GWorld at screen depth (~400KB+ at 8-bit 640x480).
 *
 * Reference: Flynn terminal_ui.c SetPortBits pattern.
 */

#ifdef GEOMYS_OFFSCREEN

#include <Quickdraw.h>
#include <Windows.h>
#include <Memory.h>
#include <Multiverse.h>
#include <string.h>

#include "offscreen.h"

#ifdef GEOMYS_COLOR
#include "color.h"

/* GWorld color offscreen state */
static GWorldPtr g_color_gworld;    /* color offscreen GWorld */
static CGrafPtr  g_saved_port;      /* saved port for GWorld */
static GDHandle  g_saved_gd;        /* saved device for GWorld */
static short     g_use_gworld;      /* 1 if using GWorld path */
#endif

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
	/* On System 7 (Color QD present), always use GWorld —
	 * even on B&W displays.  NewCWindow creates a CGrafPort,
	 * and SetPortBits corrupts CGrafPort's portPixMap field.
	 * The 8-bit GWorld + CopyBits handles all screen depths. */
	if (g_has_color_qd) {
		{
			/* If GWorld already exists, ensure it
			 * covers this window and reuse it */
			if (g_color_gworld) {
				offscreen_resize(win);
				return 1;
			}

			/* First init: create GWorld sized to
			 * this window's portRect.
			 * Force 8-bit depth so themes render
			 * correctly at any screen depth (256,
			 * Thousands, Millions).  CopyBits
			 * handles 8→N depth conversion. */
			{
				Rect bounds = win->portRect;
				QDErr err;

				err = NewGWorld(&g_color_gworld, 8,
				    &bounds, 0L, 0L, 0);
				if (err == noErr && g_color_gworld) {
					PixMapHandle ipm;
					CGrafPtr sp;
					GDHandle sd;
					Rect r;

					/* Clear GWorld to white */
					ipm = GetGWorldPixMap(
					    g_color_gworld);
					if (LockPixels(ipm)) {
						GetGWorld(&sp, &sd);
						SetGWorld(
						    g_color_gworld,
						    0L);
						r = bounds;
						EraseRect(&r);
						SetGWorld(sp, sd);
						UnlockPixels(ipm);
					}
					g_use_gworld = 1;
					g_ready = 1;
					g_active = 0;
					return 1;
				}
				/* GWorld alloc failed — fall
				 * through to 1-bit fallback */
			}
		}
	}
#endif

	if (g_offscreen_bits) {
		/* Already allocated — grow if this window is larger */
		offscreen_resize(win);
		return 1;
	}

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
#ifdef GEOMYS_COLOR
	if (g_color_gworld) {
		DisposeGWorld(g_color_gworld);
		g_color_gworld = 0L;
		g_use_gworld = 0;
	}
#endif
	if (g_offscreen_bits) {
		DisposePtr(g_offscreen_bits);
		g_offscreen_bits = 0L;
	}
	g_ready = 0;
	g_active = 0;
}

short
offscreen_begin(WindowPtr win)
{
	short win_w, win_h, buf_w, buf_h;

	if (!g_ready || g_active)
		return 0;

#ifdef GEOMYS_COLOR
	if (g_use_gworld && g_color_gworld) {
		PixMapHandle pm;
		Rect gw_bounds;

		/* Verify GWorld covers the window */
		pm = GetGWorldPixMap(g_color_gworld);
		gw_bounds = (*pm)->bounds;
		win_w = win->portRect.right - win->portRect.left;
		win_h = win->portRect.bottom - win->portRect.top;
		if (win_w > (gw_bounds.right - gw_bounds.left) ||
		    win_h > (gw_bounds.bottom - gw_bounds.top)) {
			/* Try to grow before giving up */
			offscreen_resize(win);
			pm = GetGWorldPixMap(g_color_gworld);
			gw_bounds = (*pm)->bounds;
			if (win_w > (gw_bounds.right -
			    gw_bounds.left) ||
			    win_h > (gw_bounds.bottom -
			    gw_bounds.top))
				return 0;  /* still too small */
		}

		if (LockPixels(pm)) {
			GetGWorld(&g_saved_port, &g_saved_gd);
			SetGWorld(g_color_gworld, 0L);
			g_active = 1;
			return 1;
		}
		return 0;
	}
#endif

	/* Validate buffer is large enough for this window.
	 * If the window was resized larger than the buffer,
	 * try to grow before skipping offscreen. */
	win_w = win->portRect.right - win->portRect.left;
	win_h = win->portRect.bottom - win->portRect.top;
	buf_w = (g_offscreen.rowBytes * 8);
	buf_h = g_offscreen.bounds.bottom - g_offscreen.bounds.top;
	if (win_w > buf_w || win_h > buf_h) {
		offscreen_resize(win);
		buf_w = (g_offscreen.rowBytes * 8);
		buf_h = g_offscreen.bounds.bottom -
		    g_offscreen.bounds.top;
		if (win_w > buf_w || win_h > buf_h)
			return 0;  /* still too small */
	}

	/* Save the real screen bits */
	g_saved_bits = win->portBits;

	/* Update offscreen bounds to match current window's
	 * coordinate system — critical for multi-window support
	 * where each window may have different portRect */
	g_offscreen.bounds = win->portRect;

	/* Redirect QuickDraw to offscreen */
	SetPortBits(&g_offscreen);
	g_active = 1;
	return 1;
}

void
offscreen_end(WindowPtr win, const Rect *blit_rect)
{
	if (!g_active)
		return;

#ifdef GEOMYS_COLOR
	if (g_use_gworld && g_color_gworld) {
		PixMapHandle pm;

		pm = GetGWorldPixMap(g_color_gworld);
		SetGWorld(g_saved_port, g_saved_gd);
		g_active = 0;

		/* CopyBits requires destination port fg=black,
		 * bg=white to prevent color distortion (TN QD13).
		 * Without this, themed colors left on the window
		 * cause CopyBits to colorize the transfer. */
		{
			RGBColor black = {0, 0, 0};
			RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};
			RGBForeColor(&black);
			RGBBackColor(&white);
		}

		/* Blit GWorld to window */
		CopyBits((BitMap *)*pm,
		    &((GrafPtr)win)->portBits,
		    blit_rect, blit_rect,
		    srcCopy, 0L);
		UnlockPixels(pm);
		return;
	}
#endif

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

short
offscreen_is_color(void)
{
#ifdef GEOMYS_COLOR
	return g_use_gworld;
#else
	return 0;
#endif
}

/*
 * offscreen_resize - Grow shared offscreen buffer if window exceeds it.
 *
 * The offscreen buffer is shared across all windows (cooperative
 * multitasking — only one window draws at a time). The buffer
 * grows to accommodate the largest window but never shrinks.
 * Called during window zoom/grow and from offscreen_init when a
 * new window is created.
 */
void
offscreen_resize(WindowPtr win)
{
	short pixel_w, pixel_h, rb;
	long size;
	Ptr new_bits;

#ifdef GEOMYS_COLOR
	if (g_use_gworld && g_color_gworld) {
		Rect bounds = win->portRect;
		QDErr err;
		GWorldPtr new_gw;

		/* Check if GWorld bounds cover the window */
		{
			PixMapHandle pm;
			Rect gw_bounds;

			pm = GetGWorldPixMap(g_color_gworld);
			gw_bounds = (*pm)->bounds;
			if ((bounds.right - bounds.left) <=
			    (gw_bounds.right - gw_bounds.left) &&
			    (bounds.bottom - bounds.top) <=
			    (gw_bounds.bottom - gw_bounds.top))
				return;  /* big enough */
		}

		/* Reallocate GWorld with new size (8-bit) */
		err = NewGWorld(&new_gw, 8, &bounds,
		    0L, 0L, 0);
		if (err == noErr && new_gw) {
			CGrafPtr sp;
			GDHandle sd;
			PixMapHandle npm;
			Rect r;

			DisposeGWorld(g_color_gworld);
			g_color_gworld = new_gw;

			/* Clear new GWorld to white to
			 * prevent garbage pixel blit */
			npm = GetGWorldPixMap(new_gw);
			if (LockPixels(npm)) {
				GetGWorld(&sp, &sd);
				SetGWorld(new_gw, 0L);
				r = bounds;
				EraseRect(&r);
				SetGWorld(sp, sd);
				UnlockPixels(npm);
			}
		}
		return;
	}
#endif

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
