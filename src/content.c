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
#ifdef GEOMYS_CP437
#include "cp437.h"
#include "glyphs.h"
#endif

/* Module state */
static ControlHandle g_scrollbar = 0L;
static short g_scroll_pos = 0;      /* first visible row */
static GopherState *g_page = 0L;    /* current page to render */
static short g_row_height = ROW_HEIGHT_DEFAULT;  /* dynamic row height */
static short g_font_id = 4;         /* current font (Monaco) */
static short g_font_size = 9;       /* current font size */
static CursHandle g_hand_cursor = 0L;  /* hand cursor for links */
static short g_scrolling = 0;          /* 1 during scroll action — skip offscreen */
static short g_hover_row = -1;         /* currently highlighted row, -1 = none */

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
	g_hover_row = -1;
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

#ifdef GEOMYS_CP437
/* Check if a text line contains any high bytes needing CP437 translation */
static short
cp437_has_high(const char *p, short len)
{
	short i;

	for (i = 0; i < len; i++) {
		if ((unsigned char)p[i] > 0x7F)
			return 1;
	}
	return 0;
}

/* Translate a line from CP437 to Mac Roman for text rendering.
 * dst must be at least 256 bytes. Returns output length. */
static short
cp437_translate(char *dst, const char *src, short len)
{
	short i, out = 0;

	for (i = 0; i < len && out < 255; i++) {
		unsigned char ch = (unsigned char)src[i];
		const CP437Entry *e = &cp437_table[ch];

		switch (e->method) {
		case CP437_ASCII:
		case CP437_MACROMAN:
			dst[out++] = (char)e->value;
			break;
		case CP437_GLYPH: {
			const GlyphInfo *gi =
			    glyph_get_info(e->value);
			dst[out++] = (gi && gi->copy_char) ?
			    gi->copy_char : '?';
			break;
		}
		default:
			dst[out++] = ' ';
			break;
		}
	}
	return out;
}
#endif /* GEOMYS_CP437 */

/* Draw a single directory row.
 * Self-contained — sets clip, font, erases, draws.
 * row_index is the absolute index in g_page->items[]. */
static void
content_draw_row(WindowPtr win, short row_index)
{
	Rect r, erase_r;
	short y, content_width, ellipsis_w;
	GopherItem *item;
	char line[100];
	const char *label;
	short len, text_width;
	Str255 ps;
	RgnHandle save_clip;
	extern GeomysPrefs g_prefs;

	if (!g_page || g_page->page_type != PAGE_DIRECTORY)
		return;
	if (row_index < 0 || row_index >= g_page->item_count)
		return;

	content_get_rect(win, &r);

	/* Skip if not visible */
	if (row_index < g_scroll_pos ||
	    row_index > g_scroll_pos + visible_rows(win))
		return;

	save_clip = NewRgn();
	GetClip(save_clip);
	ClipRect(&r);

	y = r.top + (row_index - g_scroll_pos + 1)
	    * g_row_height;

	SetRect(&erase_r, r.left, y - g_row_height,
	    r.right, y);
	EraseRect(&erase_r);

	TextFont(g_font_id);
	TextSize(g_font_size);
	content_width = r.right - r.left - 8;
	ellipsis_w = TextWidth("\xC9", 0, 1);

	item = &g_page->items[row_index];
	label = gopher_type_label(item->type);

	/* Find split point in display text: first run of
	 * 2+ spaces separates name from metadata */
	{
		const char *disp = item->display;
		short dlen = strlen(disp);
		short split_pos = -1;
		short name_len;
		short li;

		/* Only split non-info items — info lines are
		 * plain text that may have natural double
		 * spaces (e.g. after periods) */
		if (item->type != GOPHER_INFO) {
			for (li = 1; li < dlen - 1; li++) {
				if (disp[li] == ' ' &&
				    disp[li + 1] == ' ') {
					split_pos = li;
					break;
				}
			}
		}

		/* Determine name portion length */
		name_len = (split_pos > 0) ? split_pos : dlen;

		/* Format line with style prefix using name
		 * portion only */
		switch (g_prefs.page_style) {
		case STYLE_PLAIN:
			snprintf(line, sizeof(line),
			    "  %.*s", name_len, disp);
			break;
		case STYLE_MARKDOWN:
			if (item->type == GOPHER_INFO)
				snprintf(line, sizeof(line),
				    "  %.*s", name_len, disp);
			else if (item->type == GOPHER_SEARCH)
				snprintf(line, sizeof(line),
				    " ?  %.*s",
				    name_len, disp);
			else if (gopher_type_navigable(
			    item->type))
				snprintf(line, sizeof(line),
				    " \xA5  %.*s",
				    name_len, disp);
			else
				snprintf(line, sizeof(line),
				    "    %.*s",
				    name_len, disp);
			break;
		default: /* STYLE_TRADITIONAL */
			if (item->type == GOPHER_INFO)
				snprintf(line, sizeof(line),
				    "      %.*s",
				    name_len, disp);
			else {
#ifdef GEOMYS_GOPHER_PLUS
				if (item->has_plus)
					snprintf(line,
					    sizeof(line),
					    " %s+ %.*s", label,
					    name_len, disp);
				else
#endif
					snprintf(line,
					    sizeof(line),
					    " %s  %.*s", label,
					    name_len, disp);
			}
			break;
		}

		len = strlen(line);
		if (len > 255) len = 255;

		/* Measure name width once — reused for
		 * truncation check and metadata positioning */
		text_width = TextWidth(line, 0, len);

		/* Truncate name with ellipsis only if it
		 * alone exceeds content width */
		if (text_width > content_width && len > 3) {
			short max_w = content_width - ellipsis_w;

			while (len > 3 &&
			    TextWidth(line, 0, len) > max_w)
				len--;
			line[len] = '\xC9';
			len++;
			text_width = TextWidth(line, 0, len);
		}

		/* Draw name (left-aligned) */
		ps[0] = len;
		memcpy(ps + 1, line, len);
		MoveTo(r.left + 4, y - 2);
		DrawString(ps);

		/* Draw metadata right-aligned when Show
		 * Details is on and metadata exists */
		if (split_pos > 0 && g_prefs.show_details) {
			const char *rp = disp + split_pos;
			short right_len, right_w;
			short right_avail;
			char right_buf[100];

			/* Skip padding spaces */
			while (*rp == ' ' && rp < disp + dlen)
				rp++;
			right_len = dlen - (rp - disp);

			if (right_len > 0 && right_len < 100) {
				right_w = TextWidth(rp, 0,
				    right_len);
				right_avail = content_width -
				    text_width - 4;

				/* Truncate metadata from right
				 * with ellipsis if needed */
				if (right_w > right_avail &&
				    right_len > 1) {
					short mw = right_avail
					    - ellipsis_w;
					memcpy(right_buf, rp,
					    right_len);
					while (right_len > 1 &&
					    TextWidth(right_buf,
					    0, right_len) > mw)
						right_len--;
					right_buf[right_len] =
					    '\xC9';
					right_len++;
					right_w = TextWidth(
					    right_buf, 0,
					    right_len);
					rp = right_buf;
				}

				/* Right-align metadata */
				if (right_avail > ellipsis_w) {
					ps[0] = right_len;
					memcpy(ps + 1, rp,
					    right_len);
					MoveTo(r.right - 4 -
					    right_w, y - 2);
					DrawString(ps);
				}
			}
		}
	}

	/* Underline for hover, or always for navigable
	 * items in Plain style */
	{
		short show_underline;

		show_underline = (row_index == g_hover_row);
		if (g_prefs.page_style == STYLE_PLAIN &&
		    item->type != GOPHER_INFO &&
		    gopher_type_navigable(item->type))
			show_underline = 1;

		if (show_underline) {
			/* line now contains only prefix + name
			 * (no metadata), so underline it all */
			MoveTo(r.left + 4, y - 1);
			LineTo(r.left + 4 + TextWidth(
			    line, 0, len), y - 1);
		}
	}

	SetClip(save_clip);
	DisposeRgn(save_clip);
}

void
content_draw(WindowPtr win)
{
	Rect r, erase_r;
	short i, y, start_row, end_row;
	RgnHandle save_clip;
	short last_y;
#ifdef GEOMYS_OFFSCREEN
	short use_offscreen;
#endif

	content_get_rect(win, &r);

#ifdef GEOMYS_OFFSCREEN
	use_offscreen = offscreen_is_ready() && !g_scrolling;
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
		if (end_row > g_page->item_count)
			end_row = g_page->item_count;

		for (i = start_row; i < end_row; i++)
			content_draw_row(win, i);

		if (end_row > start_row)
			last_y = r.top + (end_row - start_row)
			    * g_row_height;
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

#ifdef GEOMYS_CP437
				if (cp437_has_high(line_start,
				    line_len)) {
					char xlated[256];
					short xlen;

					xlen = line_len;
					if (xlen > 255) xlen = 255;
					xlen = cp437_translate(
					    xlated,
					    line_start, xlen);

					if (xlen > 0 &&
					    TextWidth(xlated, 0,
					    xlen) >
					    content_width) {
						short ew =
						    TextWidth(
						    "\xC9", 0, 1);
						short mw =
						    content_width -
						    ew;
						while (xlen > 0 &&
						    TextWidth(
						    xlated, 0,
						    xlen) > mw)
							xlen--;
					}

					MoveTo(r.left + 4, y - 2);
					DrawText(xlated, 0, xlen);
					if (xlen < line_len)
						DrawChar('\xC9');
				} else
#endif
				{
				/* Truncate with ellipsis if too wide */
				if (line_len > 0 &&
				    TextWidth((Ptr)line_start, 0, line_len) > content_width) {
					short ellip_w = TextWidth("\xC9", 0, 1);
					short max_w = content_width - ellip_w;
					while (line_len > 0 &&
					    TextWidth((Ptr)line_start, 0, line_len) > max_w)
						line_len--;
				}

				MoveTo(r.left + 4, y - 2);
				DrawText((Ptr)line_start, 0,
				    line_len);

				/* Draw ellipsis if line was truncated */
				if (line_len < (p - 1 - line_start))
					DrawChar('\xC9');
				}
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

	/* Redraw grow box: erase area, then clip to the
	 * full 16x16 grow box and draw. Frame lines from
	 * DrawGrowIcon form the correct borders. */
	{
		Rect clip_r;
		RgnHandle gc = NewRgn();

		SetRect(&clip_r,
		    win->portRect.right - SCROLLBAR_WIDTH,
		    win->portRect.bottom - SCROLLBAR_WIDTH,
		    win->portRect.right,
		    win->portRect.bottom);
		EraseRect(&clip_r);

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
	if (!g_scrollbar)
		return;

	if (part == inThumb) {
		GrafPtr save;

		TrackControl(g_scrollbar, local_pt, 0L);
		g_scroll_pos = GetControlValue(g_scrollbar);
		g_hover_row = -1;

		/* Redraw after thumb release */
		GetPort(&save);
		SetPort(win);
		content_draw(win);
		SetPort(save);
	} else {
		TrackControl(g_scrollbar, local_pt, g_scroll_upp);
	}
}

static pascal void
scroll_action(ControlHandle ctl, short part)
{
	short new_pos, max_val, delta;
	WindowPtr win;
	GrafPtr save;

	new_pos = g_scroll_pos;
	max_val = GetControlMaximum(ctl);
	win = (*ctl)->contrlOwner;

	switch (part) {
	case inUpButton:
		new_pos--;
		break;
	case inDownButton:
		new_pos++;
		break;
	case inPageUp:
		new_pos -= visible_rows(win);
		break;
	case inPageDown:
		new_pos += visible_rows(win);
		break;
	default:
		return;
	}

	if (new_pos < 0)
		new_pos = 0;
	if (new_pos > max_val)
		new_pos = max_val;

	if (new_pos == g_scroll_pos)
		return;

	delta = new_pos - g_scroll_pos;
	g_scroll_pos = new_pos;
	g_hover_row = -1;  /* clear hover on scroll */
	SetControlValue(ctl, new_pos);

	GetPort(&save);
	SetPort(win);

	if ((delta == 1 || delta == -1) &&
	    g_page && g_page->page_type == PAGE_DIRECTORY) {
		/* Line scroll — use ScrollRect for speed */
		Rect cr;
		RgnHandle update_rgn = NewRgn();
		short vis = visible_rows(win);

		content_get_rect(win, &cr);
		ScrollRect(&cr, 0, -delta * g_row_height,
		    update_rgn);
		DisposeRgn(update_rgn);

		if (delta > 0) {
			content_draw_row(win,
			    g_scroll_pos + vis - 1);
			content_draw_row(win,
			    g_scroll_pos + vis);
		} else {
			content_draw_row(win, g_scroll_pos);
		}
	} else {
		/* Page/thumb scroll — full redraw */
		g_scrolling = 1;
		content_draw(win);
		g_scrolling = 0;
	}

	SetPort(save);
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
	short new_hover = -1;

	content_get_rect(win, &r);

	/* Only change cursor if mouse is in content area */
	if (!PtInRect(local_pt, &r)) {
		if (g_hover_row >= 0) {
			short old_hover = g_hover_row;

			g_hover_row = -1;
			content_draw_row(win, old_hover);
		}
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
			    gopher_type_navigable(item->type))
				new_hover = row;
		}
	}

	/* Early exit if hover unchanged — skip redraws */
	if (new_hover == g_hover_row)
		return;

	/* Update hover — redraw only affected rows */
	{
		short old_hover = g_hover_row;

		g_hover_row = new_hover;
		if (old_hover >= 0)
			content_draw_row(win, old_hover);
		if (new_hover >= 0)
			content_draw_row(win, new_hover);
	}

	/* Update cursor */
	if (new_hover >= 0) {
		if (g_hand_cursor)
			SetCursor(*g_hand_cursor);
	} else {
		InitCursor();
	}
}
