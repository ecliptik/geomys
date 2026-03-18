/*
 * content.c - Scrollable content area rendering
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>

#include "content.h"
#include "browser.h"
#include "gopher.h"
#include "gopher_types.h"
#include "main.h"
#include "settings.h"
#ifdef GEOMYS_OFFSCREEN
#include "offscreen.h"
#endif

/* Module state */
static ControlHandle g_scrollbar = 0L;
static short g_scroll_pos = 0;      /* first visible row */
static GopherState *g_page = 0L;    /* current page to render */
static short g_row_height = ROW_HEIGHT_DEFAULT;  /* dynamic row height */
static short g_font_id = 4;         /* current font (Monaco) */
static short g_font_size = 9;       /* current font size */
static CursHandle g_hand_cursor = 0L;  /* hand cursor for links */

/* Scroll bar action proc */
static pascal void scroll_action(ControlHandle ctl, short part);
static ControlActionUPP g_scroll_upp = 0L;

/* Count total rows in current page */
static short
count_rows(void)
{
	if (!g_page)
		return 0;
	if (g_page->page_type == PAGE_DIRECTORY)
		return g_page->item_count;
	if (g_page->page_type == PAGE_TEXT) {
		/* Count lines in text buffer */
		const char *p = g_page->text_buf;
		const char *end;
		short count = 0;

		if (!p)
			return 0;
		end = p + g_page->text_len;
		while (p < end) {
			if (*p == '\r')
				count++;
			p++;
		}
		if (g_page->text_len > 0 &&
		    g_page->text_buf[g_page->text_len - 1] != '\r')
			count++;  /* last line without trailing CR */
		return count;
	}
	return 0;
}

/* How many rows fit in the visible content area */
static short
visible_rows(WindowPtr win)
{
	Rect r;

	content_get_rect(win, &r);
	return (r.bottom - r.top) / g_row_height;
}

void
content_init(WindowPtr win)
{
	Rect sb_rect;
	Rect content_r;

	browser_get_content_rect(win, &content_r);

	/* Create vertical scroll bar in standard position */
	SetRect(&sb_rect,
	    win->portRect.right - SCROLLBAR_WIDTH,
	    content_r.top - 1,
	    win->portRect.right + 1,
	    win->portRect.bottom - STATUS_BAR_HEIGHT + 1);

	g_scrollbar = NewControl(win, &sb_rect, "\p", true,
	    0, 0, 0, scrollBarProc, 0L);

	g_scroll_upp = NewControlActionUPP(scroll_action);
	g_scroll_pos = 0;

	/* Load hand cursor for hovering over links */
	g_hand_cursor = GetCursor(129);

	/* Initialize font from prefs */
	{
		extern GeomysPrefs g_prefs;
		g_font_id = g_prefs.font_id;
		g_font_size = g_prefs.font_size;
		content_update_font();
	}
}

void
content_cleanup(void)
{
	if (g_scrollbar) {
		DisposeControl(g_scrollbar);
		g_scrollbar = 0L;
	}
	g_page = 0L;
}

void
content_set_page(GopherState *gs)
{
	g_page = gs;
	g_scroll_pos = 0;
}

void
content_get_rect(WindowPtr win, Rect *r)
{
	Rect browser_r;

	browser_get_content_rect(win, &browser_r);
	/* Exclude scrollbar column */
	SetRect(r, browser_r.left, browser_r.top,
	    win->portRect.right - SCROLLBAR_WIDTH,
	    browser_r.bottom);
}

void
content_update_scroll(WindowPtr win)
{
	short total, vis, max_val;

	if (!g_scrollbar)
		return;

	total = count_rows();
	vis = visible_rows(win);
	max_val = total - vis;
	if (max_val < 0)
		max_val = 0;

	SetControlMaximum(g_scrollbar, max_val);
	SetControlValue(g_scrollbar, g_scroll_pos);

	/* Dim if nothing to scroll */
	HiliteControl(g_scrollbar, max_val > 0 ? 0 : 255);
}

void
content_draw(WindowPtr win)
{
	Rect r, erase_r;
	short i, y, start_row, end_row;
	Str255 ps;
	short len;
	RgnHandle save_clip;
	short last_y;
#ifdef GEOMYS_OFFSCREEN
	short use_offscreen;
#endif

	content_get_rect(win, &r);

#ifdef GEOMYS_OFFSCREEN
	use_offscreen = offscreen_is_ready();
	if (use_offscreen)
		offscreen_begin(win);
#endif

	/* Clip to content area (excluding scrollbar) */
	save_clip = NewRgn();
	GetClip(save_clip);
	ClipRect(&r);

	/* Erase each row as we draw to reduce flicker.
	 * Full EraseRect at top causes visible flash. */

	if (!g_page) {
		EraseRect(&r);
		SetClip(save_clip);
		DisposeRgn(save_clip);
		return;
	}

	start_row = g_scroll_pos;
	end_row = start_row + visible_rows(win) + 1;
	last_y = r.top;

	if (g_page->page_type == PAGE_DIRECTORY) {
		short content_width;
		short ellipsis_w;

		TextFont(g_font_id);
		TextSize(g_font_size);
		content_width = r.right - r.left - 8;  /* 4px margin each side */
		ellipsis_w = TextWidth("\xC9", 0, 1);   /* cache for truncation */

		if (end_row > g_page->item_count)
			end_row = g_page->item_count;

		y = r.top + g_row_height;
		for (i = start_row; i < end_row; i++) {
			GopherItem *item = &g_page->items[i];
			char line[100];
			const char *label;
			short text_width;

			/* Erase this row */
			SetRect(&erase_r, r.left, y - g_row_height,
			    r.right, y);
			EraseRect(&erase_r);

			label = gopher_type_label(item->type);

			if (item->type == GOPHER_INFO)
				snprintf(line, sizeof(line),
				    "      %s", item->display);
			else
				snprintf(line, sizeof(line),
				    " %s  %s", label,
				    item->display);

			len = strlen(line);
			if (len > 255) len = 255;

			/* Truncate with ellipsis if too wide.
			 * ellipsis_w cached outside loop per Mac expert review. */
			text_width = TextWidth(line, 0, len);
			if (text_width > content_width && len > 3) {
				short max_w = content_width - ellipsis_w;
				while (len > 3 &&
				    TextWidth(line, 0, len) > max_w) {
					len--;
				}
				line[len] = '\xC9';  /* Mac Roman ellipsis */
				len++;
			}

			ps[0] = len;
			memcpy(ps + 1, line, len);

			MoveTo(r.left + 4, y);
			DrawString(ps);
			last_y = y;
			y += g_row_height;
		}
	} else if (g_page->page_type == PAGE_TEXT) {
		short content_width;

		TextFont(g_font_id);
		TextSize(g_font_size);
		content_width = r.right - r.left - 8;

		{
			const char *p = g_page->text_buf;
			const char *end_p;
			short line_idx = 0;

			if (!p) goto done;
			end_p = p + g_page->text_len;

			/* Skip to start_row */
			while (p < end_p && line_idx < start_row) {
				if (*p == '\r')
					line_idx++;
				p++;
			}

			y = r.top + g_row_height;
			while (p < end_p && line_idx < end_row) {
				const char *line_start = p;
				short line_len;

				while (p < end_p && *p != '\r')
					p++;
				line_len = p - line_start;
				if (p < end_p)
					p++;

				/* Erase this row */
				SetRect(&erase_r, r.left,
				    y - g_row_height, r.right, y);
				EraseRect(&erase_r);

				/* Truncate with ellipsis if too wide */
				if (line_len > 0 &&
				    TextWidth((Ptr)line_start, 0, line_len) > content_width) {
					short ellip_w = TextWidth("\xC9", 0, 1);
					short max_w = content_width - ellip_w;
					while (line_len > 0 &&
					    TextWidth((Ptr)line_start, 0, line_len) > max_w)
						line_len--;
				}

				MoveTo(r.left + 4, y);
				DrawText((Ptr)line_start, 0,
				    line_len);

				/* Draw ellipsis if line was truncated */
				if (line_len < (p - 1 - line_start))
					DrawChar('\xC9');
				last_y = y;
				y += g_row_height;
				line_idx++;
			}
		}
	}

	/* Erase empty area below last row */
	if (last_y < r.bottom) {
		SetRect(&erase_r, r.left, last_y,
		    r.right, r.bottom);
		EraseRect(&erase_r);
	}

done:
	SetClip(save_clip);
	DisposeRgn(save_clip);

#ifdef GEOMYS_OFFSCREEN
	/* Blit offscreen content to screen before drawing grow box */
	if (use_offscreen)
		offscreen_end(win, &r);
#endif

	/* Redraw grow box — clip to scrollbar/status bar intersection.
	 * Use SCROLLBAR_WIDTH for both dimensions so the grow box
	 * aligns perfectly with the scrollbar track. */
	{
		Rect clip_r;
		RgnHandle gc = NewRgn();

		GetClip(gc);
		SetRect(&clip_r,
		    win->portRect.right - SCROLLBAR_WIDTH,
		    win->portRect.bottom - SCROLLBAR_WIDTH,
		    win->portRect.right + 1,
		    win->portRect.bottom + 1);
		ClipRect(&clip_r);
		DrawGrowIcon(win);
		SetClip(gc);
		DisposeRgn(gc);
	}
}

Boolean
content_click(WindowPtr win, Point local_pt, GopherState *gs)
{
	Rect r;
	short clicked_row;

	content_get_rect(win, &r);
	if (!PtInRect(local_pt, &r))
		return false;

	if (!gs || gs->page_type != PAGE_DIRECTORY ||
	    gs->item_count == 0)
		return false;

	clicked_row = g_scroll_pos +
	    (local_pt.v - r.top) / g_row_height;

	if (clicked_row < 0 || clicked_row >= gs->item_count)
		return false;

	{
		GopherItem *item = &gs->items[clicked_row];
		Rect hilite_r;

		/* Info lines are not clickable */
		if (item->type == GOPHER_INFO)
			return false;

		/* Inverse highlight feedback */
		SetRect(&hilite_r,
		    r.left,
		    r.top + (clicked_row - g_scroll_pos)
		    * g_row_height,
		    r.right,
		    r.top + (clicked_row - g_scroll_pos)
		    * g_row_height + g_row_height);
		InvertRect(&hilite_r);

		/* Type 7 (Search) — show query dialog */
		if (item->type == GOPHER_SEARCH) {
			do_search_dialog(item->display,
			    item->host, item->port,
			    item->selector);
			/* Redraw to remove highlight */
			content_draw(win);
			return true;
		}

		/* Navigable types — build URI and navigate
		 * through do_navigate_url for history tracking */
		if (gopher_type_navigable(item->type)) {
			char uri[300];
			char clean_name[80];
			const char *d;
			short ni;

			gopher_build_uri(uri, sizeof(uri),
			    item->host, item->port,
			    item->type, item->selector);

			/* Extract clean name from display text:
			 * trim at first run of 2+ spaces or tab
			 * (Gopher servers pad with spaces before
			 * dates/sizes) */
			d = item->display;
			ni = 0;
			while (*d && ni < 78) {
				if (*d == '\t')
					break;
				if (*d == ' ' && *(d + 1) == ' ')
					break;
				clean_name[ni++] = *d;
				d++;
			}
			/* Trim trailing spaces */
			while (ni > 0 &&
			    clean_name[ni - 1] == ' ')
				ni--;
			clean_name[ni] = '\0';

			do_navigate_url_titled(uri,
			    clean_name[0] ? clean_name :
			    item->host);
			return true;
		}

		/* Non-navigable types — show info message */
		do_type_message(item->type, item->display,
		    item->host, item->port);
		/* Redraw to remove highlight */
		content_draw(win);
		return true;
	}

	return false;
}

void
content_scroll_click(WindowPtr win, Point local_pt, short part)
{
	(void)win;

	if (!g_scrollbar)
		return;

	if (part == inThumb) {
		TrackControl(g_scrollbar, local_pt, 0L);
		g_scroll_pos = GetControlValue(g_scrollbar);
	} else {
		TrackControl(g_scrollbar, local_pt, g_scroll_upp);
	}
}

static pascal void
scroll_action(ControlHandle ctl, short part)
{
	short new_pos, max_val;
	WindowPtr win;

	new_pos = g_scroll_pos;
	max_val = GetControlMaximum(ctl);

	switch (part) {
	case inUpButton:
		new_pos--;
		break;
	case inDownButton:
		new_pos++;
		break;
	case inPageUp:
		win = (*ctl)->contrlOwner;
		new_pos -= visible_rows(win);
		break;
	case inPageDown:
		win = (*ctl)->contrlOwner;
		new_pos += visible_rows(win);
		break;
	default:
		return;
	}

	if (new_pos < 0)
		new_pos = 0;
	if (new_pos > max_val)
		new_pos = max_val;

	if (new_pos != g_scroll_pos) {
		GrafPtr save;

		g_scroll_pos = new_pos;
		SetControlValue(ctl, new_pos);

		win = (*ctl)->contrlOwner;
		GetPort(&save);
		SetPort(win);
		content_draw(win);
		SetPort(save);
	}
}

void
content_resize(WindowPtr win)
{
	Rect sb_rect, content_r;

	if (!g_scrollbar)
		return;

	browser_get_content_rect(win, &content_r);

	SetRect(&sb_rect,
	    win->portRect.right - SCROLLBAR_WIDTH,
	    content_r.top - 1,
	    win->portRect.right + 1,
	    win->portRect.bottom - STATUS_BAR_HEIGHT + 1);

	MoveControl(g_scrollbar, sb_rect.left, sb_rect.top);
	SizeControl(g_scrollbar,
	    sb_rect.right - sb_rect.left,
	    sb_rect.bottom - sb_rect.top);

	content_update_scroll(win);
}

ControlHandle
content_get_scrollbar(void)
{
	return g_scrollbar;
}

void
content_scroll_to_top(void)
{
	g_scroll_pos = 0;
	if (g_scrollbar)
		SetControlValue(g_scrollbar, 0);
}

void
content_update_font(void)
{
	FontInfo fi;
	GrafPtr save;
	extern GeomysPrefs g_prefs;

	g_font_id = g_prefs.font_id;
	g_font_size = g_prefs.font_size;

	/* Measure row height from font metrics */
	GetPort(&save);
	if (g_window) {
		SetPort(g_window);
		TextFont(g_font_id);
		TextSize(g_font_size);
		GetFontInfo(&fi);
		g_row_height = fi.ascent + fi.descent + fi.leading;
		if (g_row_height < 10)
			g_row_height = 10;
		SetPort(save);
	}
}

short
content_row_height(void)
{
	return g_row_height;
}

void
content_cursor_update(WindowPtr win, Point local_pt)
{
	Rect r;
	short row;

	content_get_rect(win, &r);

	/* Only change cursor if mouse is in content area */
	if (!PtInRect(local_pt, &r)) {
		InitCursor();
		return;
	}

	/* Check if hovering over a navigable item */
	if (g_page && g_page->page_type == PAGE_DIRECTORY &&
	    g_page->item_count > 0) {
		row = g_scroll_pos +
		    (local_pt.v - r.top) / g_row_height;

		if (row >= 0 && row < g_page->item_count) {
			GopherItem *item = &g_page->items[row];

			if (item->type != GOPHER_INFO &&
			    gopher_type_navigable(item->type)) {
				/* Show hand cursor */
				if (g_hand_cursor)
					SetCursor(*g_hand_cursor);
				return;
			}
		}
	}

	/* Default: arrow cursor */
	InitCursor();
}
