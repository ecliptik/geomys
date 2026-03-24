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
#include "session.h"
#include "settings.h"
#include "theme.h"
#include "color.h"
#ifdef GEOMYS_CLIPBOARD
#include "clipboard.h"
#endif

extern GeomysPrefs g_prefs;

/*
 * set_system_chrome_colors - set port fg/bg to the system's window
 * content colors from the window color table (wctb). On System 7+
 * with Color QuickDraw, this respects user customization via the
 * Color control panel. Falls back to black-on-white on System 6.
 */
#ifdef GEOMYS_COLOR
static void
set_system_chrome_colors(WindowPtr win)
{
	AuxWinHandle awh;
	CTabHandle ctab;
	short i, ct_size;

	if (!g_has_color_qd)
		return;

	if (GetAuxWin(win, &awh) && awh) {
		ctab = (*awh)->awCTable;
		if (!ctab)
			return;
		ct_size = (*ctab)->ctSize;

		/* Search color table by value field —
		 * entries aren't necessarily in order */
		for (i = 0; i <= ct_size; i++) {
			if ((*ctab)->ctTable[i].value ==
			    wContentColor)
				RGBBackColor(
				    &(*ctab)->ctTable[i].rgb);
			else if ((*ctab)->ctTable[i].value ==
			    wTextColor)
				RGBForeColor(
				    &(*ctab)->ctTable[i].rgb);
		}
	}
}
#endif

/* Module state */
static TEHandle g_addr_te = 0L;
static char g_status[80];
static short g_btn_state[NAV_BTN_COUNT];
static Rect g_btn_rects[NAV_BTN_COUNT];
static Rect g_addr_rect;
static Rect g_action_rect;           /* stop/go/refresh button */
static short g_action_state = ACTION_REFRESH;
static short g_focus = FOCUS_ADDR_BAR;  /* which UI element has focus */

#ifdef GEOMYS_CLIPBOARD
/* Undo buffer — single-level toggle (undo/redo) */
static char g_undo_buf[256];
static short g_undo_len = 0;
static Boolean g_undo_valid = false;
static Boolean g_undo_is_redo = false;

static void
undo_snapshot(void)
{
	Handle text;
	short len;

	if (!g_addr_te)
		return;

	text = (*g_addr_te)->hText;
	len = (*g_addr_te)->teLength;
	if (len > (short)sizeof(g_undo_buf) - 1)
		len = sizeof(g_undo_buf) - 1;
	memcpy(g_undo_buf, *text, len);
	g_undo_len = len;
	g_undo_valid = true;
	g_undo_is_redo = false;
}
#endif

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
static void draw_home_icon(Rect *r, Boolean dim);
static void draw_refresh_icon(Rect *r, Boolean dim);
static void draw_stop_icon(Rect *r, Boolean dim);
static void draw_go_icon(Rect *r, Boolean dim);
static void draw_action_button(WindowPtr win, Boolean pressed);

/*
 * Clip TE drawing to text width so selection
 * highlight doesn't extend into empty space.
 * Saves current clip into *save_clip (caller must
 * call browser_end_addr_clip to restore).
 */
static void
browser_begin_addr_clip(RgnHandle *save_clip)
{
	Rect text_clip;
	short text_end_x, sf, ss;

	*save_clip = NewRgn();
	GetClip(*save_clip);
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
}

static void
browser_end_addr_clip(RgnHandle save_clip)
{
	SetClip(save_clip);
	DisposeRgn(save_clip);
}

void
browser_init(WindowPtr win)
{
	Rect te_rect;
	short i, x;

	/* Reset status text for new session */
	g_status[0] = '\0';

	/* Initialize button states — back/forward disabled initially */
	g_btn_state[NAV_BTN_BACK] = BTN_DISABLED;
	g_btn_state[NAV_BTN_FORWARD] = BTN_DISABLED;
	g_btn_state[NAV_BTN_HOME] = BTN_ENABLED;
	g_action_state = ACTION_REFRESH;

	/* Compute button rects */
	x = NAV_BTN_MARGIN;
	for (i = 0; i < NAV_BTN_COUNT; i++) {
		SetRect(&g_btn_rects[i], x, NAV_BTN_Y,
		    x + NAV_BTN_SIZE, NAV_BTN_Y + NAV_BTN_SIZE);
		x += NAV_BTN_SIZE + NAV_BTN_MARGIN;
	}

	/* Action button (stop/go/refresh) right of address bar */
	{
		short ax = win->portRect.right
		    - ACTION_BTN_SIZE - NAV_BTN_MARGIN;
		SetRect(&g_action_rect, ax, NAV_BTN_Y,
		    ax + ACTION_BTN_SIZE,
		    NAV_BTN_Y + ACTION_BTN_SIZE);
	}

	/* Create address bar TextEdit — frame aligns with
	 * button bounds, right edge to left of action button */
	SetRect(&te_rect, ADDR_BAR_LEFT, NAV_BTN_Y + 1,
	    g_action_rect.left - 3,
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

	/* Use system window content color for chrome background */
#ifdef GEOMYS_COLOR
	set_system_chrome_colors(win);
#endif

	/* Draw nav bar background */
	SetRect(&bar_r, 0, 0, win->portRect.right, NAV_BAR_HEIGHT);
	EraseRect(&bar_r);

	/* Draw separator line below nav bar */
	MoveTo(0, NAV_BAR_HEIGHT - 1);
	LineTo(win->portRect.right, NAV_BAR_HEIGHT - 1);

	/* Draw nav buttons */
	for (i = 0; i < NAV_BTN_COUNT; i++)
		draw_nav_button(i, false);

	/* Recalculate action button and address bar for
	 * current window width (handles resize) */
	{
		short ax = win->portRect.right
		    - ACTION_BTN_SIZE - NAV_BTN_MARGIN;
		SetRect(&g_action_rect, ax, NAV_BTN_Y,
		    ax + ACTION_BTN_SIZE,
		    NAV_BTN_Y + ACTION_BTN_SIZE);
		g_addr_rect.right = g_action_rect.left - 3;
		if (g_addr_te) {
			(*g_addr_te)->viewRect.right =
			    g_addr_rect.right;
			(*g_addr_te)->destRect.right =
			    g_addr_rect.right - 2;
		}
	}

	/* Draw action button (stop/go/refresh) */
	draw_action_button(win, false);

	/* Draw address bar frame */
	frame_r = g_addr_rect;
	InsetRect(&frame_r, -1, -1);
	FrameRect(&frame_r);

	/* Draw address bar content */
	if (g_addr_te) {
		RgnHandle save_clip;

		browser_begin_addr_clip(&save_clip);
		TEUpdate(&g_addr_rect, g_addr_te);
		browser_end_addr_clip(save_clip);
	}

	/* Restore default port colors */
#ifdef GEOMYS_COLOR
	if (g_has_color_qd) {
		RGBColor black = { 0, 0, 0 };
		RGBColor white = { 0xFFFF, 0xFFFF, 0xFFFF };
		RGBForeColor(&black);
		RGBBackColor(&white);
	}
#endif
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

/* Refresh: bold circular arrow with filled arrowhead */
static void
draw_refresh_icon(Rect *r, Boolean dim)
{
	Rect arc_r;
	short cx, cy;
	PolyHandle poly;

	cx = (r->left + r->right) / 2;
	cy = (r->top + r->bottom) / 2;

	if (dim) PenPat(&qd.gray);
	PenSize(2, 2);
	SetRect(&arc_r, cx - 5, cy - 5, cx + 5, cy + 5);
	FrameArc(&arc_r, 45, 300);
	PenSize(1, 1);

	/* Filled arrowhead at arc start (~2 o'clock),
	 * pointing down-left to follow clockwise flow */
	poly = OpenPoly();
	MoveTo(cx + 1, cy + 1);    /* tip: down-left */
	LineTo(cx + 6, cy - 4);   /* upper-right wing */
	LineTo(cx + 2, cy - 5);   /* upper-left wing */
	LineTo(cx + 1, cy + 1);
	ClosePoly();
	if (dim)
		FillPoly(poly, &qd.gray);
	else
		PaintPoly(poly);
	KillPoly(poly);
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

/* Stop: filled square (standard stop icon) */
static void
draw_stop_icon(Rect *r, Boolean dim)
{
	short cx, cy;
	Rect sq;

	cx = (r->left + r->right) / 2;
	cy = (r->top + r->bottom) / 2;

	if (dim) PenPat(&qd.gray);
	PenSize(1, 1);

	SetRect(&sq, cx - 4, cy - 4, cx + 4, cy + 4);
	if (dim)
		FillRect(&sq, &qd.gray);
	else
		PaintRect(&sq);

	PenNormal();
}

/* Go: right-pointing arrow (navigate) */
static void
draw_go_icon(Rect *r, Boolean dim)
{
	short cx, cy;
	PolyHandle poly;

	cx = (r->left + r->right) / 2;
	cy = (r->top + r->bottom) / 2;

	if (dim) PenPat(&qd.gray);
	PenSize(1, 1);

	/* Filled right arrow */
	poly = OpenPoly();
	MoveTo(cx - 4, cy - 5);
	LineTo(cx + 4, cy);
	LineTo(cx - 4, cy + 5);
	LineTo(cx - 4, cy - 5);
	ClosePoly();
	if (dim)
		FillPoly(poly, &qd.gray);
	else
		PaintPoly(poly);
	KillPoly(poly);

	PenNormal();
}

/* Draw the action button (stop/go/refresh) */
static void
draw_action_button(WindowPtr win, Boolean pressed)
{
	Rect r = g_action_rect;

	(void)win;
	EraseRect(&r);
	FrameRect(&r);

	if (pressed)
		InvertRect(&r);

	switch (g_action_state) {
	case ACTION_STOP:
		draw_stop_icon(&r, false);
		break;
	case ACTION_GO:
		draw_go_icon(&r, false);
		break;
	case ACTION_REFRESH:
		draw_refresh_icon(&r, false);
		break;
	}
}

void
browser_set_action_state(short state)
{
	g_action_state = state;
}

short
browser_get_action_state(void)
{
	return g_action_state;
}

short
status_bar_height(void)
{
	return g_prefs.show_status_bar ? STATUSBAR_HEIGHT : 0;
}

void
browser_draw_status(WindowPtr win)
{
	Rect bar_r, clip_r;
	Str255 ps;
	short len;

	/* If status bar is hidden, nothing to draw */
	if (!g_prefs.show_status_bar)
		return;

	/* Use system window content color for chrome background */
#ifdef GEOMYS_COLOR
	set_system_chrome_colors(win);
#endif

	SetRect(&bar_r, 0,
	    win->portRect.bottom - STATUSBAR_HEIGHT - SCROLLBAR_WIDTH,
	    win->portRect.right,
	    win->portRect.bottom - SCROLLBAR_WIDTH);

	/* Separator line above status bar.
	 * Stop before scrollbar column (like Flynn) so the
	 * line doesn't extend into the grow box area. */
	MoveTo(0, bar_r.top);
	LineTo(win->portRect.right - SCROLLBAR_WIDTH - 1,
	    bar_r.top);

	/* Clear status bar text area (exclude scrollbar column) */
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

	MoveTo(6, win->portRect.bottom - SCROLLBAR_WIDTH - 4);
	DrawString(ps);

	/* Restore default port colors */
#ifdef GEOMYS_COLOR
	if (g_has_color_qd) {
		RGBColor black = { 0, 0, 0 };
		RGBColor white = { 0xFFFF, 0xFFFF, 0xFFFF };
		RGBForeColor(&black);
		RGBBackColor(&white);
	}
#endif
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

#ifdef GEOMYS_CLIPBOARD
	undo_snapshot();
#endif

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

	/* Check action button (stop/go/refresh) */
	if (PtInRect(local_pt, &g_action_rect)) {
		GrafPtr save;
		Boolean in_btn = true;
		Point pt;

		GetPort(&save);
		SetPort(win);
		draw_action_button(win, true);

		while (StillDown()) {
			GetMouse(&pt);
			if (PtInRect(pt, &g_action_rect)) {
				if (!in_btn) {
					draw_action_button(win, true);
					in_btn = true;
				}
			} else {
				if (in_btn) {
					draw_action_button(win, false);
					in_btn = false;
				}
			}
		}

		draw_action_button(win, false);
		SetPort(save);

		if (!in_btn)
			return -3;
		return -4;  /* action button clicked */
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

			GetPort(&save);
			SetPort(win);

			/* Clip must be set BEFORE TEActivate */
			browser_begin_addr_clip(&save_clip);

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

			browser_end_addr_clip(save_clip);
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

#ifdef GEOMYS_CLIPBOARD
	undo_snapshot();
#endif
	TEKey(key, g_addr_te);
	return false;
}

void
browser_activate(Boolean active)
{
	RgnHandle save_clip;

	if (!g_addr_te)
		return;

	browser_begin_addr_clip(&save_clip);

	if (active)
		TEActivate(g_addr_te);
	else
		TEDeactivate(g_addr_te);

	browser_end_addr_clip(save_clip);
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
	static Boolean was_over_addr = false;

	(void)win;

	if (PtInRect(local_pt, &g_addr_rect)) {
		/* Only set I-beam once on entry to avoid
		 * interfering with TEIdle caret blink */
		if (!was_over_addr) {
			CursHandle ibeam;

			ibeam = GetCursor(iBeamCursor);
			if (ibeam)
				SetCursor(*ibeam);
			was_over_addr = true;
		}
		return true;
	}
	was_over_addr = false;
	return false;
}

void
browser_get_content_rect(WindowPtr win, Rect *r)
{
	SetRect(r, 0, NAV_BAR_HEIGHT,
	    win->portRect.right,
	    win->portRect.bottom - status_bar_height() -
	    SCROLLBAR_WIDTH);
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
	undo_snapshot();
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
	undo_snapshot();
	system_to_te_scrap();
	TEPaste(g_addr_te);
}

void
browser_edit_clear(void)
{
	if (!g_addr_te)
		return;
	undo_snapshot();
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

void
browser_edit_undo(void)
{
	char swap[256];
	short swap_len;
	Handle text;
	short len;

	if (!g_addr_te || !g_undo_valid)
		return;

	/* Save current text for redo */
	text = (*g_addr_te)->hText;
	len = (*g_addr_te)->teLength;
	if (len > (short)sizeof(swap) - 1)
		len = sizeof(swap) - 1;
	memcpy(swap, *text, len);
	swap_len = len;

	/* Replace with undo buffer */
	TESetSelect(0, 32767, g_addr_te);
	TEDelete(g_addr_te);
	TEInsert(g_undo_buf, g_undo_len, g_addr_te);

	/* Swap: old text becomes new undo buffer (for redo) */
	memcpy(g_undo_buf, swap, swap_len);
	g_undo_len = swap_len;
	g_undo_is_redo = !g_undo_is_redo;
}

Boolean
browser_can_undo(void)
{
	return g_undo_valid;
}

Boolean
browser_is_redo(void)
{
	return g_undo_is_redo;
}
#endif /* GEOMYS_CLIPBOARD */

/*
 * Save/restore browser chrome statics to/from session struct.
 * Used during session switching (GEOMYS_MAX_WINDOWS > 1).
 */
void
browser_save_state(struct BrowserSession *s)
{
	s->addr_te = g_addr_te;
	memcpy(s->status, g_status, sizeof(g_status));
	memcpy(s->btn_state, g_btn_state, sizeof(g_btn_state));
	memcpy(s->btn_rects, g_btn_rects, sizeof(g_btn_rects));
	s->addr_rect = g_addr_rect;
	s->focus = g_focus;
}

void
browser_load_state(struct BrowserSession *s)
{
	g_addr_te = s->addr_te;
	memcpy(g_status, s->status, sizeof(g_status));
	memcpy(g_btn_state, s->btn_state, sizeof(g_btn_state));
	memcpy(g_btn_rects, s->btn_rects, sizeof(g_btn_rects));
	g_addr_rect = s->addr_rect;
	g_focus = s->focus;
}
