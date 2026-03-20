/*
 * browser.c - Browser chrome: nav bar, address bar, status bar
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>

#include "browser.h"
#include "content.h"
#include "main.h"
#ifdef GEOMYS_CLIPBOARD
#include "clipboard.h"
#endif

/* Module state */
static TEHandle g_addr_te = 0L;
static char g_status[80];
static short g_btn_state[NAV_BTN_COUNT];
static Rect g_btn_rects[NAV_BTN_COUNT];
static Rect g_addr_rect;
static short g_focus = FOCUS_ADDR_BAR;  /* which UI element has focus */

short
browser_get_focus(void)
{
	return g_focus;
}

void
browser_set_focus(short focus)
{
	g_focus = focus;
}

/* Forward declarations */
static void draw_nav_bar(WindowPtr win);
static void draw_nav_button(short btn_id, Boolean pressed);
static void draw_back_icon(Rect *r, Boolean dim);
static void draw_forward_icon(Rect *r, Boolean dim);
static void draw_refresh_icon(Rect *r, Boolean dim);
static void draw_home_icon(Rect *r, Boolean dim);

void
browser_init(WindowPtr win)
{
	Rect te_rect;
	short i, x;

	/* Initialize button states — back/forward disabled initially */
	g_btn_state[NAV_BTN_BACK] = BTN_DISABLED;
	g_btn_state[NAV_BTN_FORWARD] = BTN_DISABLED;
	g_btn_state[NAV_BTN_REFRESH] = BTN_ENABLED;
	g_btn_state[NAV_BTN_HOME] = BTN_ENABLED;

	/* Compute button rects */
	x = NAV_BTN_MARGIN;
	for (i = 0; i < NAV_BTN_COUNT; i++) {
		SetRect(&g_btn_rects[i], x, NAV_BTN_Y,
		    x + NAV_BTN_SIZE, NAV_BTN_Y + NAV_BTN_SIZE);
		x += NAV_BTN_SIZE + NAV_BTN_MARGIN;
	}

	/* Create address bar TextEdit — frame aligns with
	 * button bounds, right edge aligns with scrollbar */
	SetRect(&te_rect, ADDR_BAR_LEFT, NAV_BTN_Y + 1,
	    win->portRect.right - SCROLLBAR_WIDTH,
	    NAV_BTN_Y + NAV_BTN_SIZE - 1);
	g_addr_rect = te_rect;
	{
		Rect dest_rect = te_rect;
		/* Symmetric inset for vertical centering —
		 * keeps selection highlight evenly padded */
		dest_rect.top += 1;
		dest_rect.bottom -= 1;
		g_addr_te = TENew(&dest_rect, &te_rect);
	}
	if (g_addr_te) {
		(*g_addr_te)->txFont = 4;  /* Monaco */
		(*g_addr_te)->txSize = 12;
	}

	strcpy(g_status, "Ready");
}

void
browser_cleanup(void)
{
	if (g_addr_te) {
		TEDispose(g_addr_te);
		g_addr_te = 0L;
	}
}

void
browser_draw(WindowPtr win)
{
	draw_nav_bar(win);
	browser_draw_status(win);
}

static void
draw_nav_bar(WindowPtr win)
{
	Rect bar_r, frame_r;
	short i;

	/* Draw nav bar background */
	SetRect(&bar_r, 0, 0, win->portRect.right, NAV_BAR_HEIGHT);
	EraseRect(&bar_r);

	/* Draw separator line below nav bar */
	MoveTo(0, NAV_BAR_HEIGHT - 1);
	LineTo(win->portRect.right, NAV_BAR_HEIGHT - 1);

	/* Draw nav buttons */
	for (i = 0; i < NAV_BTN_COUNT; i++)
		draw_nav_button(i, false);

	/* Draw address bar frame */
	frame_r = g_addr_rect;
	InsetRect(&frame_r, -1, -1);
	FrameRect(&frame_r);

	/* Draw address bar content */
	if (g_addr_te) {
		RgnHandle save_clip = NewRgn();
		Rect text_clip;
		short text_end_x;
		short save_font, save_size;

		/* Clip TE drawing to text width so selection
		 * highlight doesn't extend into empty space */
		GetClip(save_clip);
		save_font = qd.thePort->txFont;
		save_size = qd.thePort->txSize;
		TextFont((*g_addr_te)->txFont);
		TextSize((*g_addr_te)->txSize);
		text_end_x = (*g_addr_te)->destRect.left +
		    TextWidth(*((*g_addr_te)->hText), 0,
		    (*g_addr_te)->teLength) + 1;
		TextFont(save_font);
		TextSize(save_size);
		text_clip = g_addr_rect;
		if (text_end_x < text_clip.right)
			text_clip.right = text_end_x;
		ClipRect(&text_clip);

		TEUpdate(&g_addr_rect, g_addr_te);

		SetClip(save_clip);
		DisposeRgn(save_clip);
	}
}

static void
draw_nav_button(short btn_id, Boolean pressed)
{
	Rect r = g_btn_rects[btn_id];
	Boolean dim = (g_btn_state[btn_id] == BTN_DISABLED);

	/* Draw button frame */
	FrameRect(&r);

	if (pressed) {
		InvertRect(&r);
		return;
	}

	/* Erase interior */
	{
		Rect inner = r;
		InsetRect(&inner, 1, 1);
		EraseRect(&inner);
	}

	/* Draw icon */
	switch (btn_id) {
	case NAV_BTN_BACK:
		draw_back_icon(&r, dim);
		break;
	case NAV_BTN_FORWARD:
		draw_forward_icon(&r, dim);
		break;
	case NAV_BTN_REFRESH:
		draw_refresh_icon(&r, dim);
		break;
	case NAV_BTN_HOME:
		draw_home_icon(&r, dim);
		break;
	}
}

/* Back arrow: filled left-pointing triangle with frame for clean edges */
static void
draw_back_icon(Rect *r, Boolean dim)
{
	short cx, cy;
	PolyHandle poly;

	cx = (r->left + r->right) / 2;
	cy = (r->top + r->bottom) / 2;

	if (dim) PenPat(&qd.gray);
	PenSize(1, 1);

	poly = OpenPoly();
	MoveTo(cx + 4, cy - 5);
	LineTo(cx - 3, cy);
	LineTo(cx + 4, cy + 5);
	LineTo(cx + 4, cy - 5);
	ClosePoly();
	if (dim)
		FillPoly(poly, &qd.gray);
	else
		PaintPoly(poly);
	FramePoly(poly);
	KillPoly(poly);

	PenNormal();
}

/* Forward arrow: filled right-pointing triangle with frame for clean edges */
static void
draw_forward_icon(Rect *r, Boolean dim)
{
	short cx, cy;
	PolyHandle poly;

	cx = (r->left + r->right) / 2;
	cy = (r->top + r->bottom) / 2;

	if (dim) PenPat(&qd.gray);
	PenSize(1, 1);

	poly = OpenPoly();
	MoveTo(cx - 4, cy - 5);
	LineTo(cx + 3, cy);
	LineTo(cx - 4, cy + 5);
	LineTo(cx - 4, cy - 5);
	ClosePoly();
	if (dim)
		FillPoly(poly, &qd.gray);
	else
		PaintPoly(poly);
	FramePoly(poly);
	KillPoly(poly);

	PenNormal();
}

/* Refresh: circular arrow with arrowhead */
static void
draw_refresh_icon(Rect *r, Boolean dim)
{
	Rect arc_r;
	short cx, cy;

	cx = (r->left + r->right) / 2;
	cy = (r->top + r->bottom) / 2;

	SetRect(&arc_r, cx - 5, cy - 5, cx + 5, cy + 5);
	if (dim) PenPat(&qd.gray);
	PenSize(1, 1);
	FrameArc(&arc_r, 30, 300);
	/* Arrow head at top-right of arc */
	MoveTo(cx + 5, cy - 1);
	LineTo(cx + 5, cy - 5);
	LineTo(cx + 1, cy - 5);
	PenNormal();
}

/* Home: filled house shape */
static void
draw_home_icon(Rect *r, Boolean dim)
{
	short cx, cy;
	PolyHandle poly;
	Rect door_r;

	cx = (r->left + r->right) / 2;
	cy = (r->top + r->bottom) / 2;

	if (dim) PenPat(&qd.gray);
	PenSize(1, 1);

	/* Filled roof triangle */
	poly = OpenPoly();
	MoveTo(cx - 7, cy);
	LineTo(cx, cy - 6);
	LineTo(cx + 7, cy);
	LineTo(cx - 7, cy);
	ClosePoly();
	if (dim)
		FillPoly(poly, &qd.gray);
	else
		PaintPoly(poly);
	KillPoly(poly);

	/* Filled walls */
	{
		Rect wall_r;
		SetRect(&wall_r, cx - 5, cy, cx + 5, cy + 6);
		if (dim)
			FillRect(&wall_r, &qd.gray);
		else
			PaintRect(&wall_r);
	}

	/* Door cutout (white) */
	SetRect(&door_r, cx - 2, cy + 2, cx + 2, cy + 6);
	EraseRect(&door_r);

	PenNormal();
}

void
browser_draw_status(WindowPtr win)
{
	Rect bar_r, clip_r;
	Str255 ps;
	short len;
	RgnHandle save_clip;

	SetRect(&bar_r, 0, win->portRect.bottom - STATUS_BAR_HEIGHT,
	    win->portRect.right, win->portRect.bottom);

	/* Separator line above status bar.
	 * Stop before scrollbar column (like Flynn) so the
	 * line doesn't extend into the grow box area. */
	MoveTo(0, bar_r.top);
	LineTo(win->portRect.right - SCROLLBAR_WIDTH - 1,
	    bar_r.top);

	/* Clear status bar text area (exclude grow box corner) */
	bar_r.top += 1;
	SetRect(&clip_r, bar_r.left, bar_r.top,
	    win->portRect.right - SCROLLBAR_WIDTH, bar_r.bottom);
	EraseRect(&clip_r);

	TextFont(3);  /* Geneva */
	TextSize(9);

	len = strlen(g_status);
	if (len > 255) len = 255;
	ps[0] = len;
	memcpy(ps + 1, g_status, len);

	MoveTo(6, win->portRect.bottom - 4);
	DrawString(ps);

	/* Redraw grow box after status bar.
	 * Erase first, then clip to full grow box and draw. */
	save_clip = NewRgn();
	GetClip(save_clip);
	SetRect(&clip_r,
	    win->portRect.right - SCROLLBAR_WIDTH,
	    win->portRect.bottom - SCROLLBAR_WIDTH,
	    win->portRect.right,
	    win->portRect.bottom);
	EraseRect(&clip_r);
	SetRect(&clip_r,
	    win->portRect.right - SCROLLBAR_WIDTH,
	    win->portRect.bottom - SCROLLBAR_WIDTH,
	    win->portRect.right + 1,
	    win->portRect.bottom + 1);
	ClipRect(&clip_r);
	DrawGrowIcon(win);
	SetClip(save_clip);
	DisposeRgn(save_clip);
}

void
browser_set_status(const char *msg)
{
	strncpy(g_status, msg, sizeof(g_status) - 1);
	g_status[sizeof(g_status) - 1] = '\0';
}

void
browser_set_url(const char *url)
{
	Str255 ps;
	short len;

	if (!g_addr_te)
		return;

	/* Select all and replace */
	TESetSelect(0, 32767, g_addr_te);
	len = strlen(url);
	if (len > 255) len = 255;
	ps[0] = len;
	memcpy(ps + 1, url, len);
	TEDelete(g_addr_te);
	TEInsert(url, len, g_addr_te);
}

void
browser_get_url(char *buf, short buf_size)
{
	Handle text;
	short len;

	if (!g_addr_te) {
		buf[0] = '\0';
		return;
	}

	text = (*g_addr_te)->hText;
	len = (*g_addr_te)->teLength;
	if (len >= buf_size)
		len = buf_size - 1;
	memcpy(buf, *text, len);
	buf[len] = '\0';
}

void
browser_set_button_state(short btn_id, short state)
{
	if (btn_id >= 0 && btn_id < NAV_BTN_COUNT)
		g_btn_state[btn_id] = state;
}

short
browser_click(WindowPtr win, Point local_pt)
{
	short i;

	/* Check nav buttons */
	for (i = 0; i < NAV_BTN_COUNT; i++) {
		if (PtInRect(local_pt, &g_btn_rects[i])) {
			if (g_btn_state[i] == BTN_DISABLED)
				return -3;

			/* Visual feedback — invert while tracking */
			{
				GrafPtr save;
				Boolean in_btn = true;
				Point pt;

				GetPort(&save);
				SetPort(win);
				draw_nav_button(i, true);

				while (StillDown()) {
					GetMouse(&pt);
					if (PtInRect(pt,
					    &g_btn_rects[i])) {
						if (!in_btn) {
							draw_nav_button(
							    i, true);
							in_btn = true;
						}
					} else {
						if (in_btn) {
							draw_nav_button(
							    i, false);
							in_btn = false;
						}
					}
				}

				draw_nav_button(i, false);
				SetPort(save);

				if (!in_btn)
					return -3;  /* released outside */
			}

			return i;  /* button ID */
		}
	}

	/* Check address bar */
	if (PtInRect(local_pt, &g_addr_rect)) {
		g_focus = FOCUS_ADDR_BAR;
#ifdef GEOMYS_CLIPBOARD
		/* Clear content selection when switching to addr bar */
		if (content_has_selection())
			content_clear_selection(win);
#endif
		if (g_addr_te) {
			GrafPtr save;
			static unsigned long last_click = 0;
			unsigned long now;
			RgnHandle save_clip;
			Rect text_clip;
			short text_end_x;

			GetPort(&save);
			SetPort(win);

			/* Clip TE drawing to text width so
			 * selection doesn't fill empty space.
			 * Must be set BEFORE TEActivate. */
			save_clip = NewRgn();
			GetClip(save_clip);
			{
				short sf, ss;

				sf = qd.thePort->txFont;
				ss = qd.thePort->txSize;
				TextFont((*g_addr_te)->txFont);
				TextSize((*g_addr_te)->txSize);
				text_end_x = (*g_addr_te)->
				    destRect.left +
				    TextWidth(*((*g_addr_te)->
				    hText), 0,
				    (*g_addr_te)->teLength) + 1;
				TextFont(sf);
				TextSize(ss);
			}
			text_clip = g_addr_rect;
			if (text_end_x < text_clip.right)
				text_clip.right = text_end_x;
			ClipRect(&text_clip);

			TEActivate(g_addr_te);

			/* Double-click: select all text */
			now = TickCount();
			if (last_click &&
			    (now - last_click) <
			    LMGetDoubleTime()) {
				short len;

				len = (*g_addr_te)->teLength;
				TESetSelect(0, len, g_addr_te);
				last_click = 0;
			} else {
				TEClick(local_pt, false,
				    g_addr_te);
				last_click = now;
			}

			SetClip(save_clip);
			DisposeRgn(save_clip);
			SetPort(save);
		}
		return -2;
	}

	return -1;  /* not in chrome */
}

Boolean
browser_key(WindowPtr win, EventRecord *event)
{
	char key;

	(void)win;

	if (!g_addr_te)
		return false;

	key = event->message & charCodeMask;

	/* Return/Enter in address bar triggers navigation */
	if (key == '\r' || key == '\n' || key == 0x03)
		return true;  /* signal to caller to navigate */

	TEKey(key, g_addr_te);
	return false;
}

void
browser_activate(Boolean active)
{
	RgnHandle save_clip;
	Rect text_clip;
	short text_end_x, sf, ss;

	if (!g_addr_te)
		return;

	/* Clip TE drawing to text width so deactivate
	 * highlight doesn't extend into empty space */
	save_clip = NewRgn();
	GetClip(save_clip);
	sf = qd.thePort->txFont;
	ss = qd.thePort->txSize;
	TextFont((*g_addr_te)->txFont);
	TextSize((*g_addr_te)->txSize);
	text_end_x = (*g_addr_te)->destRect.left +
	    TextWidth(*((*g_addr_te)->hText), 0,
	    (*g_addr_te)->teLength) + 1;
	TextFont(sf);
	TextSize(ss);
	text_clip = g_addr_rect;
	if (text_end_x < text_clip.right)
		text_clip.right = text_end_x;
	ClipRect(&text_clip);

	if (active)
		TEActivate(g_addr_te);
	else
		TEDeactivate(g_addr_te);

	SetClip(save_clip);
	DisposeRgn(save_clip);
}

void
browser_idle(void)
{
	if (g_addr_te)
		TEIdle(g_addr_te);
}

Boolean
browser_cursor_update(WindowPtr win, Point local_pt)
{
	(void)win;

	if (PtInRect(local_pt, &g_addr_rect)) {
		CursHandle ibeam;

		ibeam = GetCursor(iBeamCursor);
		if (ibeam)
			SetCursor(*ibeam);
		return true;
	}
	return false;
}

void
browser_get_content_rect(WindowPtr win, Rect *r)
{
	SetRect(r, 0, NAV_BAR_HEIGHT,
	    win->portRect.right,
	    win->portRect.bottom - STATUS_BAR_HEIGHT);
}

#ifdef GEOMYS_CLIPBOARD
/* Copy TE scrap to system scrap */
static void
te_to_system_scrap(void)
{
	Handle scrap_h;
	long len;

	scrap_h = LMGetTEScrpHandle();
	len = (long)LMGetTEScrpLength();
	if (scrap_h && len > 0) {
		HLock(scrap_h);
		ZeroScrap();
		PutScrap(len, 'TEXT', *scrap_h);
		HUnlock(scrap_h);
	}
}

/* Copy system scrap to TE scrap */
static void
system_to_te_scrap(void)
{
	Handle h, te_h;
	long offset, len;

	h = NewHandle(0);
	if (!h)
		return;
	len = GetScrap(h, 'TEXT', &offset);
	if (len > 0) {
		te_h = LMGetTEScrpHandle();
		SetHandleSize(te_h, len);
		if (MemError() == noErr) {
			HLock(h);
			HLock(te_h);
			BlockMove(*h, *te_h, len);
			HUnlock(te_h);
			HUnlock(h);
			LMSetTEScrpLength((short)len);
		}
	}
	DisposeHandle(h);
}

void
browser_edit_cut(void)
{
	if (!g_addr_te)
		return;
	TECut(g_addr_te);
	te_to_system_scrap();
}

void
browser_edit_copy(void)
{
	if (!g_addr_te)
		return;
	TECopy(g_addr_te);
	te_to_system_scrap();
}

void
browser_edit_paste(void)
{
	if (!g_addr_te)
		return;
	system_to_te_scrap();
	TEPaste(g_addr_te);
}

void
browser_edit_clear(void)
{
	if (!g_addr_te)
		return;
	TEDelete(g_addr_te);
}

void
browser_edit_select_all(void)
{
	if (!g_addr_te)
		return;
	TESetSelect(0, 32767, g_addr_te);
}

Boolean
browser_has_selection(void)
{
	if (!g_addr_te)
		return false;
	return (*g_addr_te)->selStart != (*g_addr_te)->selEnd;
}
#endif /* GEOMYS_CLIPBOARD */
