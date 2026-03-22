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
#include "session.h"
#include "theme.h"
#include "color.h"
#ifdef GEOMYS_OFFSCREEN
#include "offscreen.h"
#endif
#ifdef GEOMYS_CP437
#include "cp437.h"
#include "glyphs.h"
#endif

/* Module state */
static ControlHandle g_scrollbar = 0L;
static ControlHandle g_hscrollbar = 0L;
static short g_scroll_pos = 0;      /* first visible row */
static short g_hscroll_pos = 0;     /* first visible pixel column */
static short g_content_max_width = 0; /* max line width in pixels */
static GopherState *g_page = 0L;    /* current page to render */
static short g_row_height = ROW_HEIGHT_DEFAULT;  /* dynamic row height */
static short g_font_id = 4;         /* current font (Monaco) */
static short g_font_size = 9;       /* current font size */
static CursHandle g_hand_cursor = 0L;  /* hand cursor for links */
#ifdef GEOMYS_CLIPBOARD
static CursHandle g_ibeam_cursor = 0L; /* I-beam cursor for text */
#endif
static short g_scrolling = 0;          /* 1 during scroll action — skip offscreen */
static short g_hover_row = -1;         /* currently highlighted row, -1 = none */
static short g_in_full_draw = 0;       /* 1 during content_draw loop — skip per-row clip/restore */

#ifdef GEOMYS_CLIPBOARD
/* Selection state */
static Selection g_sel;
static short g_win_active = 1;  /* window active flag for selection dimming */

/* Forward declarations for selection helpers */
short content_row_text(short row, char *buf, short bufsiz);
static short pixel_to_col(short row, short pixel_x, WindowPtr win);
static short col_to_pixel(short row, short col, WindowPtr win);
static void sel_normalize(short *sr, short *sc, short *er, short *ec);
#endif

/* Scroll bar action procs */
static pascal void scroll_action(ControlHandle ctl, short part);
static pascal void hscroll_action(ControlHandle ctl, short part);
static ControlActionUPP g_scroll_upp = 0L;
static ControlActionUPP g_hscroll_upp = 0L;

/* Forward declaration */
static void content_calc_max_width(WindowPtr win);
static void content_update_hscroll(WindowPtr win);

/* Count total rows in current page */
static short
count_rows(void)
{
	if (!g_page)
		return 0;
	if (g_page->page_type == PAGE_DIRECTORY)
		return g_page->item_count;
	if (g_page->page_type == PAGE_TEXT)
		return g_page->text_line_count;
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

	/* Create vertical scroll bar — extends alongside both
	 * content area and status bar, ends at grow box.
	 * The +1 shares border pixel with grow box top. */
	SetRect(&sb_rect,
	    win->portRect.right - SCROLLBAR_WIDTH,
	    content_r.top - 1,
	    win->portRect.right + 1,
	    win->portRect.bottom - SCROLLBAR_WIDTH + 1);

	g_scrollbar = NewControl(win, &sb_rect, "\p", true,
	    0, 0, 0, scrollBarProc, 0L);

	g_scroll_upp = NewControlActionUPP(scroll_action);
	g_scroll_pos = 0;

	/* Create horizontal scroll bar — below status bar,
	 * left of grow box. Matches Finder scrollbar geometry:
	 * top aligns with grow box top, right frame shares
	 * border pixel with grow box left edge. */
	SetRect(&sb_rect,
	    -1,
	    win->portRect.bottom - SCROLLBAR_WIDTH,
	    win->portRect.right - SCROLLBAR_WIDTH + 1,
	    win->portRect.bottom + 1);

	g_hscrollbar = NewControl(win, &sb_rect, "\p", true,
	    0, 0, 0, scrollBarProc, 0L);

	g_hscroll_upp = NewControlActionUPP(hscroll_action);
	g_hscroll_pos = 0;
	g_content_max_width = 0;

	/* Load hand cursor for hovering over links */
	g_hand_cursor = GetCursor(129);
#ifdef GEOMYS_CLIPBOARD
	/* Load I-beam cursor for text selection */
	g_ibeam_cursor = GetCursor(iBeamCursor);
#endif

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
	if (g_hscrollbar) {
		DisposeControl(g_hscrollbar);
		g_hscrollbar = 0L;
	}
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
	g_hscroll_pos = 0;
	g_content_max_width = 0;
	g_hover_row = -1;
#ifdef GEOMYS_CLIPBOARD
	/* Clear selection on page change */
	g_sel.active = 0;
	g_sel.selecting = 0;
	g_sel.word_mode = 0;
	g_sel.last_click_ticks = 0;
#endif
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

	/* Update horizontal scrollbar */
	content_update_hscroll(win);
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
	short y, content_width;
	GopherItem *item;
	char line[100];
	const char *label;
	short len, text_width;
	Str255 ps;
	RgnHandle save_clip = 0L;
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

	if (!g_in_full_draw) {
		save_clip = NewRgn();
		GetClip(save_clip);
		ClipRect(&r);
	}

	y = r.top + (row_index - g_scroll_pos + 1)
	    * g_row_height;

	SetRect(&erase_r, r.left, y - g_row_height,
	    r.right, y);

#ifdef GEOMYS_THEMES
	{
		const ThemeColors *t = theme_current();
		short did_erase = 0;

		if (t) {
#ifdef GEOMYS_COLOR
			if (g_has_color_qd) {
				/* Color path: set RGB background */
				if (row_index == g_hover_row)
					theme_set_bg(&t->hover_bg);
				else
					theme_set_bg(&t->bg);
				EraseRect(&erase_r);
				did_erase = 1;

				/* Set foreground per item type */
				{
					GopherItem *ti =
					    &g_page->items[row_index];
					if (ti->type == GOPHER_INFO)
						theme_set_fg(&t->text);
					else if (ti->type == GOPHER_ERROR)
						theme_set_fg(&t->link_error);
					else if (ti->type == GOPHER_SEARCH)
						theme_set_fg(&t->link_search);
					else if (gopher_type_navigable(
					    ti->type))
						theme_set_fg(&t->link);
					else
						theme_set_fg(
						    &t->link_external);
				}
			} else
#endif
			if (t->is_dark) {
				/* Mono dark: black bg, white text */
				PaintRect(&erase_r);
				TextMode(srcBic);
				did_erase = 1;
			}
		}
		if (!did_erase)
			EraseRect(&erase_r);
	}
#else
	EraseRect(&erase_r);
#endif

	TextFont(g_font_id);
	TextSize(g_font_size);
	content_width = r.right - r.left - 8;

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

		/* Measure name width */
		text_width = TextWidth(line, 0, len);

		/* Draw with horizontal offset —
		 * ClipRect handles clipping automatically.
		 * Traditional style: draw label prefix in
		 * label color, then name in link color. */
#if defined(GEOMYS_THEMES) && defined(GEOMYS_COLOR)
		if (g_has_color_qd &&
		    g_prefs.page_style == STYLE_TRADITIONAL &&
		    item->type != GOPHER_INFO) {
			const ThemeColors *lt =
			    theme_current();
			if (lt) {
				/* Label prefix is 5 chars:
				 * " DIR " or " TXT+" etc */
				short lbl_len = 5;
				Str255 lps;

#ifdef GEOMYS_GOPHER_PLUS
				if (item->has_plus)
					lbl_len = 5;
#endif
				if (lbl_len > len)
					lbl_len = len;

				/* Draw label in label color */
				theme_set_fg(&lt->label);
				lps[0] = lbl_len;
				memcpy(lps + 1, line,
				    lbl_len);
				MoveTo(r.left + 4 -
				    g_hscroll_pos, y - 2);
				DrawString(lps);

				/* Draw name in link color */
				{
					GopherItem *ci =
					    &g_page->items[
					    row_index];
					if (ci->type ==
					    GOPHER_ERROR)
						theme_set_fg(
						    &lt->link_error);
					else if (ci->type ==
					    GOPHER_SEARCH)
						theme_set_fg(
						    &lt->link_search);
					else if (
					    gopher_type_navigable(
					    ci->type))
						theme_set_fg(
						    &lt->link);
					else
						theme_set_fg(
						    &lt->
						    link_external);
				}
				if (len > lbl_len) {
					ps[0] = len - lbl_len;
					memcpy(ps + 1,
					    line + lbl_len,
					    len - lbl_len);
					DrawString(ps);
				}
			} else {
				ps[0] = len;
				memcpy(ps + 1, line, len);
				MoveTo(r.left + 4 -
				    g_hscroll_pos, y - 2);
				DrawString(ps);
			}
		} else
#endif
		{
			ps[0] = len;
			memcpy(ps + 1, line, len);
			MoveTo(r.left + 4 - g_hscroll_pos,
			    y - 2);
			DrawString(ps);
		}

		/* Draw metadata right-aligned when Show
		 * Details is on and metadata exists.
		 * Metadata stays fixed at right edge
		 * (not affected by hscroll). */
		if (split_pos > 0 && g_prefs.show_details) {
			const char *rp = disp + split_pos;
			short right_len, right_w;
			short right_avail;
			short ellipsis_w;

			ellipsis_w = TextWidth("\xC9", 0, 1);

			/* Skip padding spaces */
			while (*rp == ' ' && rp < disp + dlen)
				rp++;
			right_len = dlen - (rp - disp);

			if (right_len > 0 && right_len < 100) {
				char right_buf[100];

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
#ifdef GEOMYS_THEMES
					{
						const ThemeColors *mt =
						    theme_current();
#ifdef GEOMYS_COLOR
						if (mt && g_has_color_qd)
							theme_set_fg(
							    &mt->metadata);
#endif
					}
#endif
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
			MoveTo(r.left + 4 - g_hscroll_pos,
			    y - 1);
			LineTo(r.left + 4 - g_hscroll_pos +
			    TextWidth(line, 0, len), y - 1);
		}
	}

#ifdef GEOMYS_CLIPBOARD
	/* Invert selected characters (skip when window inactive) */
	if (g_sel.active && g_win_active) {
		short sr, sc, er, ec;

		sr = g_sel.anchor_row;
		sc = g_sel.anchor_col;
		er = g_sel.extent_row;
		ec = g_sel.extent_col;
		sel_normalize(&sr, &sc, &er, &ec);

		if (row_index >= sr && row_index <= er) {
			Rect inv_r;
			short x1, x2;

			if (sr == er) {
				/* Single row: invert col range */
				x1 = col_to_pixel(row_index,
				    sc, win);
				x2 = col_to_pixel(row_index,
				    ec, win);
			} else if (row_index == sr) {
				/* First row: from start_col to EOL */
				x1 = col_to_pixel(row_index,
				    sc, win);
				x2 = r.right;
			} else if (row_index == er) {
				/* Last row: from BOL to end_col */
				x1 = r.left;
				x2 = col_to_pixel(row_index,
				    ec, win);
			} else {
				/* Middle row: full width */
				x1 = r.left;
				x2 = r.right;
			}

			if (x2 > x1) {
				SetRect(&inv_r, x1,
				    y - g_row_height, x2, y);
#ifdef GEOMYS_COLOR
				if (g_has_color_qd &&
				    !theme_is_dark()) {
					/* Light themes: use HiliteMode
					 * for system highlight color.
					 * Dark themes: plain XOR works
					 * better on colored bgs. */
					LMSetHiliteMode(
					    LMGetHiliteMode()
					    & ~(1 << pHiliteBit));
				}
#endif
				InvertRect(&inv_r);
			}
		}
	}
#endif

#ifdef GEOMYS_THEMES
	/* Restore normal text mode if dark theme used srcBic */
	if (theme_is_dark() && !theme_is_color())
		TextMode(srcOr);
#endif

	/* Restore port colors after themed row draw — prevents
	 * theme colors leaking into subsequent draws (e.g. during
	 * track_content_drag which calls this per-row) */
	if (!g_in_full_draw)
		theme_restore_colors();

	if (!g_in_full_draw) {
		SetClip(save_clip);
		DisposeRgn(save_clip);
	}
}

/* Draw a single text row by index.
 * Self-contained — sets clip, font, erases, draws.
 * Uses text_lines[] for O(1) offset lookup. */
static void
content_draw_text_row(WindowPtr win, short line_index)
{
	Rect r, erase_r;
	short y;
	const char *line_start;
	short line_len;
	RgnHandle save_clip = 0L;

	if (!g_page || g_page->page_type != PAGE_TEXT)
		return;
	if (!g_page->text_buf || !g_page->text_lines)
		return;
	if (line_index < 0 ||
	    line_index >= g_page->text_line_count)
		return;

	content_get_rect(win, &r);

	/* Skip if not visible */
	if (line_index < g_scroll_pos ||
	    line_index > g_scroll_pos + visible_rows(win))
		return;

	if (!g_in_full_draw) {
		save_clip = NewRgn();
		GetClip(save_clip);
		ClipRect(&r);
	}

	y = r.top + (line_index - g_scroll_pos + 1)
	    * g_row_height;

	/* Erase this row */
	SetRect(&erase_r, r.left, y - g_row_height,
	    r.right, y);

#ifdef GEOMYS_THEMES
	{
		const ThemeColors *t = theme_current();
		short did_erase = 0;

		if (t) {
#ifdef GEOMYS_COLOR
			if (g_has_color_qd) {
				theme_set_bg(&t->bg);
				theme_set_fg(&t->text);
				EraseRect(&erase_r);
				did_erase = 1;
			} else
#endif
			if (t->is_dark) {
				PaintRect(&erase_r);
				TextMode(srcBic);
				did_erase = 1;
			}
		}
		if (!did_erase)
			EraseRect(&erase_r);
	}
#else
	EraseRect(&erase_r);
#endif

	TextFont(g_font_id);
	TextSize(g_font_size);

	/* Get line start and length from index */
	line_start = g_page->text_buf +
	    g_page->text_lines[line_index];
	if (line_index + 1 < g_page->text_line_count) {
		/* Line ends at \r before next line start */
		line_len = (short)(g_page->text_lines[
		    line_index + 1] -
		    g_page->text_lines[line_index] - 1);
	} else {
		/* Last line — ends at text_len */
		line_len = (short)(g_page->text_len -
		    g_page->text_lines[line_index]);
		/* Strip trailing \r if present */
		if (line_len > 0 &&
		    line_start[line_len - 1] == '\r')
			line_len--;
	}
	if (line_len < 0)
		line_len = 0;

#ifdef GEOMYS_CP437
	if (cp437_has_high(line_start, line_len)) {
		char xlated[256];
		short xlen;

		xlen = line_len;
		if (xlen > 255) xlen = 255;
		xlen = cp437_translate(xlated,
		    line_start, xlen);

		/* Draw with horizontal offset — ClipRect
		 * handles clipping automatically */
		MoveTo(r.left + 4 - g_hscroll_pos, y - 2);
		DrawText(xlated, 0, xlen);
	} else
#endif
	{
		/* Draw with horizontal offset — ClipRect
		 * handles clipping automatically */
		MoveTo(r.left + 4 - g_hscroll_pos, y - 2);
		DrawText((Ptr)line_start, 0, line_len);
	}

#ifdef GEOMYS_CLIPBOARD
	/* Invert selected characters (skip when window inactive) */
	if (g_sel.active && g_win_active) {
		short sr, sc, er, ec;

		sr = g_sel.anchor_row;
		sc = g_sel.anchor_col;
		er = g_sel.extent_row;
		ec = g_sel.extent_col;
		sel_normalize(&sr, &sc, &er, &ec);

		if (line_index >= sr && line_index <= er) {
			Rect inv_r;
			short x1, x2;

			if (sr == er) {
				x1 = col_to_pixel(line_index,
				    sc, win);
				x2 = col_to_pixel(line_index,
				    ec, win);
			} else if (line_index == sr) {
				x1 = col_to_pixel(line_index,
				    sc, win);
				x2 = r.right;
			} else if (line_index == er) {
				x1 = r.left;
				x2 = col_to_pixel(line_index,
				    ec, win);
			} else {
				x1 = r.left;
				x2 = r.right;
			}

			if (x2 > x1) {
				SetRect(&inv_r, x1,
				    y - g_row_height, x2, y);
#ifdef GEOMYS_COLOR
				if (g_has_color_qd &&
				    !theme_is_dark()) {
					/* Light themes: use HiliteMode
					 * for system highlight color.
					 * Dark themes: plain XOR works
					 * better on colored bgs. */
					LMSetHiliteMode(
					    LMGetHiliteMode()
					    & ~(1 << pHiliteBit));
				}
#endif
				InvertRect(&inv_r);
			}
		}
	}
#endif

#ifdef GEOMYS_THEMES
	/* Restore normal text mode if dark theme used srcBic */
	if (theme_is_dark() && !theme_is_color())
		TextMode(srcOr);
#endif

	/* Restore port colors after themed row draw — prevents
	 * theme colors leaking into subsequent draws (e.g. during
	 * track_content_drag which calls this per-row) */
	if (!g_in_full_draw)
		theme_restore_colors();

	if (!g_in_full_draw) {
		SetClip(save_clip);
		DisposeRgn(save_clip);
	}
}

void
content_draw(WindowPtr win)
{
	Rect r, erase_r;
	short i, start_row, end_row;
	RgnHandle save_clip;
	short last_y;
#ifdef GEOMYS_OFFSCREEN
	short use_offscreen;
#endif

	content_get_rect(win, &r);

#ifdef GEOMYS_THEMES
	theme_reset_cache();
#endif

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
#ifdef GEOMYS_THEMES
		{
			const ThemeColors *t = theme_current();
			if (t) {
#ifdef GEOMYS_COLOR
				if (g_has_color_qd)
					theme_set_bg(&t->bg);
#endif
			}
		}
#endif
		EraseRect(&r);
		theme_restore_colors();
		SetClip(save_clip);
		DisposeRgn(save_clip);
		return;
	}

	start_row = g_scroll_pos;
	end_row = start_row + visible_rows(win) + 1;
	last_y = r.top;

	g_in_full_draw = 1;

	if (g_page->page_type == PAGE_DIRECTORY) {
		if (end_row > g_page->item_count)
			end_row = g_page->item_count;

		for (i = start_row; i < end_row; i++)
			content_draw_row(win, i);

		if (end_row > start_row)
			last_y = r.top + (end_row - start_row)
			    * g_row_height;
	} else if (g_page->page_type == PAGE_TEXT) {
		short text_end;

		if (!g_page->text_buf || !g_page->text_lines)
			goto done;

		text_end = g_page->text_line_count;
		if (end_row > text_end)
			end_row = text_end;

		for (i = start_row; i < end_row; i++)
			content_draw_text_row(win, i);

		if (end_row > start_row)
			last_y = r.top + (end_row - start_row)
			    * g_row_height;
	}

	g_in_full_draw = 0;

	/* Erase empty area below last row */
	if (last_y < r.bottom) {
		SetRect(&erase_r, r.left, last_y,
		    r.right, r.bottom);
#ifdef GEOMYS_THEMES
		{
			const ThemeColors *t = theme_current();
			if (t) {
#ifdef GEOMYS_COLOR
				if (g_has_color_qd)
					theme_set_bg(&t->bg);
				else
#endif
				if (t->is_dark) {
					PaintRect(&erase_r);
					goto skip_tail_erase;
				}
			}
		}
#endif
		EraseRect(&erase_r);
#ifdef GEOMYS_THEMES
	skip_tail_erase:
		;
#endif
	}

done:
	g_in_full_draw = 0;
	/* Restore port colors to black/white so themed colors
	 * don't leak into chrome (nav bar, address bar, buttons) */
	theme_restore_colors();

	SetClip(save_clip);
	DisposeRgn(save_clip);

#ifdef GEOMYS_OFFSCREEN
	/* Blit offscreen content to screen before drawing grow box */
	if (use_offscreen)
		offscreen_end(win, &r);
#endif

}

#ifdef GEOMYS_CLIPBOARD
/* Selection slop threshold in pixels — movement beyond this
 * turns a click into a drag/selection */
#define SEL_SLOP  3

/* Text drawing offset from content rect left edge */
#define TEXT_X_OFFSET  4

/*
 * content_row_text - get the display text for a given row.
 * For directory: formatted prefix+name (matching draw logic).
 * For text: raw line text.
 * Returns length of text placed in buf.
 */
short
content_row_text(short row, char *buf, short bufsiz)
{
	extern GeomysPrefs g_prefs;

	if (!g_page)
		return 0;

	if (g_page->page_type == PAGE_DIRECTORY) {
		GopherItem *item;
		const char *disp, *label;
		short dlen, split_pos, name_len, li, len;

		if (row < 0 || row >= g_page->item_count)
			return 0;

		item = &g_page->items[row];
		label = gopher_type_label(item->type);
		disp = item->display;
		dlen = strlen(disp);
		split_pos = -1;

		if (item->type != GOPHER_INFO) {
			for (li = 1; li < dlen - 1; li++) {
				if (disp[li] == ' ' &&
				    disp[li + 1] == ' ') {
					split_pos = li;
					break;
				}
			}
		}

		name_len = (split_pos > 0) ?
		    split_pos : dlen;

		switch (g_prefs.page_style) {
		case STYLE_PLAIN:
			len = snprintf(buf, bufsiz,
			    "  %.*s", name_len, disp);
			break;
		case STYLE_MARKDOWN:
			if (item->type == GOPHER_INFO)
				len = snprintf(buf, bufsiz,
				    "  %.*s", name_len, disp);
			else if (item->type == GOPHER_SEARCH)
				len = snprintf(buf, bufsiz,
				    " ?  %.*s",
				    name_len, disp);
			else if (gopher_type_navigable(
			    item->type))
				len = snprintf(buf, bufsiz,
				    " \xA5  %.*s",
				    name_len, disp);
			else
				len = snprintf(buf, bufsiz,
				    "    %.*s",
				    name_len, disp);
			break;
		default: /* STYLE_TRADITIONAL */
			if (item->type == GOPHER_INFO)
				len = snprintf(buf, bufsiz,
				    "      %.*s",
				    name_len, disp);
			else {
#ifdef GEOMYS_GOPHER_PLUS
				if (item->has_plus)
					len = snprintf(buf,
					    bufsiz,
					    " %s+ %.*s", label,
					    name_len, disp);
				else
#endif
					len = snprintf(buf,
					    bufsiz,
					    " %s  %.*s", label,
					    name_len, disp);
			}
			break;
		}

		if (len < 0) len = 0;
		if (len >= bufsiz) len = bufsiz - 1;
		return len;

	} else if (g_page->page_type == PAGE_TEXT) {
		const char *line_start;
		short line_len;

		if (row < 0 ||
		    row >= g_page->text_line_count)
			return 0;
		if (!g_page->text_buf ||
		    !g_page->text_lines)
			return 0;

		line_start = g_page->text_buf +
		    g_page->text_lines[row];
		if (row + 1 < g_page->text_line_count) {
			line_len = (short)(
			    g_page->text_lines[row + 1] -
			    g_page->text_lines[row] - 1);
		} else {
			line_len = (short)(g_page->text_len -
			    g_page->text_lines[row]);
			if (line_len > 0 &&
			    line_start[line_len - 1] == '\r')
				line_len--;
		}
		if (line_len < 0) line_len = 0;
		if (line_len >= bufsiz)
			line_len = bufsiz - 1;

		memcpy(buf, line_start, line_len);
		buf[line_len] = '\0';
		return line_len;
	}

	return 0;
}

/*
 * pixel_to_col - convert pixel x offset to character column.
 * Uses CharWidth to accumulate widths; returns nearest
 * character boundary. Must be called with correct port/font.
 */
static short
pixel_to_col(short row, short pixel_x, WindowPtr win)
{
	char buf[256];
	short len, i, accum, cw;
	Rect r;

	content_get_rect(win, &r);
	pixel_x -= (r.left + TEXT_X_OFFSET);
	pixel_x += g_hscroll_pos;  /* adjust for hscroll */
	if (pixel_x <= 0)
		return 0;

	len = content_row_text(row, buf, sizeof(buf));
	if (len <= 0)
		return 0;

	TextFont(g_font_id);
	TextSize(g_font_size);

	accum = 0;
	for (i = 0; i < len; i++) {
		cw = CharWidth(buf[i]);
		if (accum + cw / 2 > pixel_x)
			return i;
		accum += cw;
	}
	return len;
}

/*
 * col_to_pixel - convert character column to pixel x.
 * Returns x in local coordinates. Must have correct font set.
 */
static short
col_to_pixel(short row, short col, WindowPtr win)
{
	char buf[256];
	short len;
	Rect r;

	content_get_rect(win, &r);
	len = content_row_text(row, buf, sizeof(buf));
	if (col <= 0 || len <= 0)
		return r.left + TEXT_X_OFFSET;
	if (col > len)
		col = len;

	TextFont(g_font_id);
	TextSize(g_font_size);

	return r.left + TEXT_X_OFFSET - g_hscroll_pos +
	    TextWidth(buf, 0, col);
}

/* Normalize selection: ensure start <= end in reading order */
static void
sel_normalize(short *sr, short *sc, short *er, short *ec)
{
	if (*sr > *er ||
	    (*sr == *er && *sc > *ec)) {
		short tmp;

		tmp = *sr; *sr = *er; *er = tmp;
		tmp = *sc; *sc = *ec; *ec = tmp;
	}
}

/* Check if ticks are within double-click time */
static short
is_double_click(unsigned long now, short row, short col)
{
	unsigned long dbltime;

	dbltime = LMGetDoubleTime();
	return (g_sel.last_click_ticks &&
	    (now - g_sel.last_click_ticks) < dbltime &&
	    row == g_sel.last_click_row &&
	    (col - g_sel.last_click_col <= 2) &&
	    (g_sel.last_click_col - col <= 2));
}

/*
 * sel_find_word_bounds - find word boundaries at column
 * position on a row. Scans for contiguous non-space run.
 * Sets start_col and end_col (end is one past last char).
 */
static void
sel_find_word_bounds(short row, short col,
    short *start_col, short *end_col)
{
	char buf[256];
	short len, i;

	len = content_row_text(row, buf, sizeof(buf));
	if (len <= 0 || col >= len) {
		*start_col = 0;
		*end_col = len > 0 ? len : 0;
		return;
	}

	/* If clicked on a space, select just that space */
	if (buf[col] == ' ') {
		*start_col = col;
		*end_col = col + 1;
		return;
	}

	/* Scan left for word start */
	i = col;
	while (i > 0 && buf[i - 1] != ' ')
		i--;
	*start_col = i;

	/* Scan right for word end */
	i = col;
	while (i < len && buf[i] != ' ')
		i++;
	*end_col = i;
}

/* Begin tracking selection drag. Redraws rows as selection
 * extends. Returns 1 if the mouse moved beyond slop
 * (i.e. user is dragging), 0 if released without moving
 * (i.e. a click). Tracks character-level positions. */
static short
track_content_drag(WindowPtr win, Point start_pt, short start_row)
{
	Point pt;
	short row, col, prev_row, prev_col;
	short total;
	short dragged = 0;
	Rect r;

	content_get_rect(win, &r);
	total = count_rows();
	prev_row = start_row;
	prev_col = g_sel.anchor_col;

	while (StillDown()) {
		GetMouse(&pt);

		/* Check slop */
		if (!dragged) {
			short dx = pt.h - start_pt.h;
			short dy = pt.v - start_pt.v;

			if (dx < 0) dx = -dx;
			if (dy < 0) dy = -dy;
			if (dx < SEL_SLOP && dy < SEL_SLOP)
				continue;
			dragged = 1;
			g_hover_row = -1;  /* clear hover */
		}

		row = g_scroll_pos +
		    (pt.v - r.top) / g_row_height;
		if (row < 0) row = 0;
		if (row >= total) row = total - 1;

		col = pixel_to_col(row, pt.h, win);

		if (row == prev_row && col == prev_col)
			continue;

		/* Update extent and redraw changed rows */
		{
			short old_sr, old_sc, old_er, old_ec;
			short new_sr, new_sc, new_er, new_ec;
			short rr, lo, hi;

			/* Old visible range */
			old_sr = g_sel.anchor_row;
			old_sc = g_sel.anchor_col;
			old_er = g_sel.extent_row;
			old_ec = g_sel.extent_col;
			sel_normalize(&old_sr, &old_sc,
			    &old_er, &old_ec);

			if (g_sel.word_mode) {
				/* In word mode, extend by
				 * word boundaries */
				if (row < g_sel.word_anchor_start ||
				    (row == g_sel.word_anchor_start &&
				    col < g_sel.anchor_col)) {
					g_sel.extent_row = row;
					g_sel.extent_col = col;
				} else {
					g_sel.extent_row = row;
					g_sel.extent_col = col;
				}
			} else {
				g_sel.extent_row = row;
				g_sel.extent_col = col;
			}

			/* New visible range */
			new_sr = g_sel.anchor_row;
			new_sc = g_sel.anchor_col;
			new_er = g_sel.extent_row;
			new_ec = g_sel.extent_col;
			sel_normalize(&new_sr, &new_sc,
			    &new_er, &new_ec);

			/* Redraw changed rows */
			lo = old_sr < new_sr ?
			    old_sr : new_sr;
			hi = old_er > new_er ?
			    old_er : new_er;
			for (rr = lo; rr <= hi; rr++) {
				if (g_page->page_type ==
				    PAGE_DIRECTORY)
					content_draw_row(
					    win, rr);
				else
					content_draw_text_row(
					    win, rr);
			}
		}

		prev_row = row;
		prev_col = col;
	}

	return dragged;
}

/* Handle a click that was determined to be navigation
 * (not a drag/selection) on a directory row. */
static Boolean
do_directory_navigate(WindowPtr win, GopherState *gs,
    short clicked_row)
{
	Rect r;
	GopherItem *item;
	Rect hilite_r;
	short y_off;

	content_get_rect(win, &r);
	item = &gs->items[clicked_row];

	/* Info lines are not clickable */
	if (item->type == GOPHER_INFO)
		return false;

	/* Inverse highlight feedback */
	y_off = (clicked_row - g_scroll_pos) * g_row_height;
	SetRect(&hilite_r,
	    r.left, r.top + y_off,
	    r.right, r.top + y_off + g_row_height);
	InvertRect(&hilite_r);

	/* Type 7 (Search) — show query dialog */
	if (item->type == GOPHER_SEARCH) {
		do_search_dialog(item->display,
		    item->host, item->port,
		    item->selector);
		content_draw(win);
		return true;
	}

	/* Navigable types — build URI and navigate */
	if (gopher_type_navigable(item->type)) {
		char uri[300];
		char clean_name[80];
		const char *d;
		short ni;

		gopher_build_uri(uri, sizeof(uri),
		    item->host, item->port,
		    item->type, item->selector);

		/* Extract clean name from display text */
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
		while (ni > 0 && clean_name[ni - 1] == ' ')
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
	content_draw(win);
	return true;
}
#endif /* GEOMYS_CLIPBOARD */

Boolean
content_click(WindowPtr win, Point local_pt, GopherState *gs)
{
	Rect r;
	short clicked_row;
	short total;

	content_get_rect(win, &r);
	if (!PtInRect(local_pt, &r))
		return false;

	browser_set_focus(FOCUS_CONTENT);
#ifdef GEOMYS_CLIPBOARD
	/* Deactivate addr bar cursor when focus moves to content */
	browser_activate(false);
#endif

	if (!gs || gs->page_type == PAGE_NONE)
		return false;

	total = count_rows();
	if (total == 0)
		return false;

	clicked_row = g_scroll_pos +
	    (local_pt.v - r.top) / g_row_height;
	if (clicked_row < 0 || clicked_row >= total)
		return false;

#ifdef GEOMYS_CLIPBOARD
	{
		unsigned long now = TickCount();
		short clicked_col;
		short was_double;
		short dragged;

		clicked_col = pixel_to_col(clicked_row,
		    local_pt.h, win);

		/* Clear previous selection */
		if (g_sel.active)
			content_clear_selection(win);

		/* Check for double-click */
		was_double = is_double_click(now,
		    clicked_row, clicked_col);

		if (was_double) {
			/* Double-click: word selection */
			short wsc, wec;

			sel_find_word_bounds(clicked_row,
			    clicked_col, &wsc, &wec);
			g_sel.active = 1;
			g_sel.selecting = 1;
			g_sel.anchor_row = clicked_row;
			g_sel.anchor_col = wsc;
			g_sel.extent_row = clicked_row;
			g_sel.extent_col = wec;
			g_sel.word_mode = 1;
			g_sel.word_anchor_start = wsc;
			g_sel.word_anchor_end = wec;
			g_hover_row = -1;

			/* Redraw to show word highlight */
			if (gs->page_type == PAGE_DIRECTORY)
				content_draw_row(win,
				    clicked_row);
			else
				content_draw_text_row(win,
				    clicked_row);

			/* Track drag extension */
			track_content_drag(win, local_pt,
			    clicked_row);
			g_sel.selecting = 0;

			/* Record for next double-click */
			g_sel.last_click_ticks = now;
			g_sel.last_click_row = clicked_row;
			g_sel.last_click_col = clicked_col;
			return false;
		}

		/* Single click — start selection and track */
		g_sel.active = 1;
		g_sel.selecting = 1;
		g_sel.anchor_row = clicked_row;
		g_sel.anchor_col = clicked_col;
		g_sel.extent_row = clicked_row;
		g_sel.extent_col = clicked_col;
		g_sel.word_mode = 0;

		dragged = track_content_drag(win, local_pt,
		    clicked_row);
		g_sel.selecting = 0;

		/* Record for double-click detection */
		g_sel.last_click_ticks = now;
		g_sel.last_click_row = clicked_row;
		g_sel.last_click_col = clicked_col;

		if (dragged) {
			/* Mouse moved — selection drag */
			return false;
		}

		/* No drag — this was a click. Clear the
		 * proto-selection. */
		g_sel.active = 0;

		/* For directory pages, navigate */
		if (gs->page_type == PAGE_DIRECTORY)
			return do_directory_navigate(win, gs,
			    clicked_row);

		/* Text pages: click does nothing (no nav) */
		return false;
	}
#else
	/* Non-clipboard build: original behavior */
	if (gs->page_type != PAGE_DIRECTORY)
		return false;

	{
		GopherItem *item = &gs->items[clicked_row];
		Rect hilite_r;

		if (item->type == GOPHER_INFO)
			return false;

		SetRect(&hilite_r,
		    r.left,
		    r.top + (clicked_row - g_scroll_pos)
		    * g_row_height,
		    r.right,
		    r.top + (clicked_row - g_scroll_pos)
		    * g_row_height + g_row_height);
		InvertRect(&hilite_r);

		if (item->type == GOPHER_SEARCH) {
			do_search_dialog(item->display,
			    item->host, item->port,
			    item->selector);
			content_draw(win);
			return true;
		}

		if (gopher_type_navigable(item->type)) {
			char uri[300];
			char clean_name[80];
			const char *d;
			short ni;

			gopher_build_uri(uri, sizeof(uri),
			    item->host, item->port,
			    item->type, item->selector);

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
			while (ni > 0 &&
			    clean_name[ni - 1] == ' ')
				ni--;
			clean_name[ni] = '\0';

			do_navigate_url_titled(uri,
			    clean_name[0] ? clean_name :
			    item->host);
			return true;
		}

		do_type_message(item->type, item->display,
		    item->host, item->port);
		content_draw(win);
		return true;
	}
#endif /* GEOMYS_CLIPBOARD */
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
	    g_page && (g_page->page_type == PAGE_DIRECTORY ||
	    g_page->page_type == PAGE_TEXT)) {
		/* Line scroll — use ScrollRect for speed */
		Rect cr;
		RgnHandle update_rgn = NewRgn();
		short vis = visible_rows(win);

		content_get_rect(win, &cr);
		ScrollRect(&cr, 0, -delta * g_row_height,
		    update_rgn);
		DisposeRgn(update_rgn);

		if (g_page->page_type == PAGE_DIRECTORY) {
			if (delta > 0) {
				content_draw_row(win,
				    g_scroll_pos + vis - 1);
				content_draw_row(win,
				    g_scroll_pos + vis);
			} else {
				content_draw_row(win,
				    g_scroll_pos);
			}
		} else {
			if (delta > 0) {
				content_draw_text_row(win,
				    g_scroll_pos + vis - 1);
				content_draw_text_row(win,
				    g_scroll_pos + vis);
			} else {
				content_draw_text_row(win,
				    g_scroll_pos);
			}
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

	browser_get_content_rect(win, &content_r);

	/* Reposition vertical scrollbar — extends alongside
	 * content and status bar, ends at grow box */
	if (g_scrollbar) {
		SetRect(&sb_rect,
		    win->portRect.right - SCROLLBAR_WIDTH,
		    content_r.top - 1,
		    win->portRect.right + 1,
		    win->portRect.bottom - SCROLLBAR_WIDTH + 1);

		MoveControl(g_scrollbar, sb_rect.left,
		    sb_rect.top);
		SizeControl(g_scrollbar,
		    sb_rect.right - sb_rect.left,
		    sb_rect.bottom - sb_rect.top);
	}

	/* Reposition horizontal scrollbar — below status bar,
	 * left of grow box */
	if (g_hscrollbar) {
		SetRect(&sb_rect,
		    -1,
		    win->portRect.bottom - SCROLLBAR_WIDTH,
		    win->portRect.right - SCROLLBAR_WIDTH + 1,
		    win->portRect.bottom + 1);

		MoveControl(g_hscrollbar, sb_rect.left,
		    sb_rect.top);
		SizeControl(g_hscrollbar,
		    sb_rect.right - sb_rect.left,
		    sb_rect.bottom - sb_rect.top);
	}

	content_update_scroll(win);
}

ControlHandle
content_get_scrollbar(void)
{
	return g_scrollbar;
}

ControlHandle
content_get_hscrollbar(void)
{
	return g_hscrollbar;
}

void
content_scroll_to_top(void)
{
	g_scroll_pos = 0;
	g_hscroll_pos = 0;
	if (g_scrollbar)
		SetControlValue(g_scrollbar, 0);
	if (g_hscrollbar)
		SetControlValue(g_hscrollbar, 0);
}

short
content_get_scroll_pos(void)
{
	return g_scroll_pos;
}

void
content_set_scroll_pos(short pos)
{
	short max_val;

	if (!g_scrollbar)
		return;

	max_val = GetControlMaximum(g_scrollbar);
	if (pos < 0)
		pos = 0;
	if (pos > max_val)
		pos = max_val;

	g_scroll_pos = pos;
	SetControlValue(g_scrollbar, pos);
}

/*
 * content_vscroll_by - scroll vertically by delta rows.
 * Positive = down, negative = up. Clamps to [0, max].
 */
void
content_vscroll_by(short delta)
{
	short new_pos, max_val;
	WindowPtr win;
	GrafPtr save;

	if (!g_scrollbar)
		return;

	max_val = GetControlMaximum(g_scrollbar);
	new_pos = g_scroll_pos + delta;

	if (new_pos < 0)
		new_pos = 0;
	if (new_pos > max_val)
		new_pos = max_val;

	if (new_pos == g_scroll_pos)
		return;

	g_scroll_pos = new_pos;
	SetControlValue(g_scrollbar, new_pos);

	win = (*g_scrollbar)->contrlOwner;
	GetPort(&save);
	SetPort(win);
	g_scrolling = 1;
	content_draw(win);
	g_scrolling = 0;
	SetPort(save);
}

/*
 * content_visible_rows - return number of rows visible in
 * the content area.
 */
short
content_visible_rows(void)
{
	WindowPtr win;

	if (!g_scrollbar)
		return 1;

	win = (*g_scrollbar)->contrlOwner;
	return visible_rows(win);
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

void
content_recalc_width(WindowPtr win)
{
	content_calc_max_width(win);
	content_update_hscroll(win);
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

	/* Update hover — redraw only affected rows */
	if (new_hover != g_hover_row) {
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
#ifdef GEOMYS_CLIPBOARD
	} else if (g_page && g_page->page_type == PAGE_TEXT) {
		/* I-beam over text pages */
		if (g_ibeam_cursor)
			SetCursor(*g_ibeam_cursor);
	} else if (g_page && g_page->page_type == PAGE_DIRECTORY) {
		/* I-beam over non-navigable items */
		row = g_scroll_pos +
		    (local_pt.v - r.top) / g_row_height;
		if (row >= 0 && row < g_page->item_count &&
		    !gopher_type_navigable(
		    g_page->items[row].type)) {
			if (g_ibeam_cursor)
				SetCursor(*g_ibeam_cursor);
		} else {
			InitCursor();
		}
#endif
	} else {
		InitCursor();
	}
}

#ifdef GEOMYS_CLIPBOARD
void
content_activate(WindowPtr win, Boolean active)
{
	g_win_active = active ? 1 : 0;

	/* Redraw to update selection highlight appearance */
	if (g_sel.active && win) {
		GrafPtr save;

		GetPort(&save);
		SetPort(win);
		content_draw(win);
		SetPort(save);
	}
}

Boolean
content_has_selection(void)
{
	return g_sel.active;
}

void
content_clear_selection(WindowPtr win)
{
	short old_start, old_end;

	if (!g_sel.active)
		return;

	/* Remember range for redraw */
	if (g_sel.anchor_row <= g_sel.extent_row) {
		old_start = g_sel.anchor_row;
		old_end = g_sel.extent_row;
	} else {
		old_start = g_sel.extent_row;
		old_end = g_sel.anchor_row;
	}

	g_sel.active = 0;
	g_sel.selecting = 0;
	g_sel.anchor_row = 0;
	g_sel.anchor_col = 0;
	g_sel.extent_row = 0;
	g_sel.extent_col = 0;
	g_sel.word_mode = 0;

	/* Redraw affected rows */
	if (win && g_page) {
		short i;
		GrafPtr save;

		GetPort(&save);
		SetPort(win);
		for (i = old_start; i <= old_end; i++) {
			if (g_page->page_type == PAGE_DIRECTORY)
				content_draw_row(win, i);
			else if (g_page->page_type == PAGE_TEXT)
				content_draw_text_row(win, i);
		}
		SetPort(save);
	}
}

Boolean
content_get_selection(short *start_row, short *start_col,
    short *end_row, short *end_col)
{
	if (!g_sel.active)
		return false;

	*start_row = g_sel.anchor_row;
	*start_col = g_sel.anchor_col;
	*end_row = g_sel.extent_row;
	*end_col = g_sel.extent_col;
	sel_normalize(start_row, start_col,
	    end_row, end_col);
	return true;
}

void
content_select_all(WindowPtr win)
{
	short total;
	char buf[256];

	total = count_rows();
	if (total == 0)
		return;

	g_sel.active = 1;
	g_sel.selecting = 0;
	g_sel.anchor_row = 0;
	g_sel.anchor_col = 0;
	g_sel.extent_row = total - 1;
	g_sel.extent_col = content_row_text(total - 1,
	    buf, sizeof(buf));
	g_sel.word_mode = 0;
	g_hover_row = -1;

	if (win) {
		GrafPtr save;

		GetPort(&save);
		SetPort(win);
		content_draw(win);
		SetPort(save);
	}
}
#endif /* GEOMYS_CLIPBOARD */

/*
 * content_calc_max_width - measure the widest line in the page.
 * Called after page load and font changes to set hscroll range.
 */
static void
content_calc_max_width(WindowPtr win)
{
	short i, total, w;
	GrafPtr save;

	g_content_max_width = 0;

	if (!g_page)
		return;

	GetPort(&save);
	SetPort(win);
	TextFont(g_font_id);
	TextSize(g_font_size);

	total = count_rows();

	if (g_page->page_type == PAGE_DIRECTORY) {
		char line[100];
		extern GeomysPrefs g_prefs;

		for (i = 0; i < total; i++) {
			GopherItem *item = &g_page->items[i];
			const char *disp = item->display;
			const char *label;
			short dlen = strlen(disp);
			short split_pos = -1;
			short name_len, len, li;

			label = gopher_type_label(item->type);

			if (item->type != GOPHER_INFO) {
				for (li = 1; li < dlen - 1;
				    li++) {
					if (disp[li] == ' ' &&
					    disp[li + 1] == ' ') {
						split_pos = li;
						break;
					}
				}
			}

			name_len = (split_pos > 0) ?
			    split_pos : dlen;

			switch (g_prefs.page_style) {
			case STYLE_PLAIN:
				snprintf(line, sizeof(line),
				    "  %.*s",
				    name_len, disp);
				break;
			case STYLE_MARKDOWN:
				if (item->type == GOPHER_INFO)
					snprintf(line,
					    sizeof(line),
					    "  %.*s",
					    name_len, disp);
				else if (item->type ==
				    GOPHER_SEARCH)
					snprintf(line,
					    sizeof(line),
					    " ?  %.*s",
					    name_len, disp);
				else if (gopher_type_navigable(
				    item->type))
					snprintf(line,
					    sizeof(line),
					    " \xA5  %.*s",
					    name_len, disp);
				else
					snprintf(line,
					    sizeof(line),
					    "    %.*s",
					    name_len, disp);
				break;
			default:
				if (item->type == GOPHER_INFO)
					snprintf(line,
					    sizeof(line),
					    "      %.*s",
					    name_len, disp);
				else {
#ifdef GEOMYS_GOPHER_PLUS
					if (item->has_plus)
						snprintf(line,
						    sizeof(line),
						    " %s+ %.*s",
						    label,
						    name_len,
						    disp);
					else
#endif
						snprintf(line,
						    sizeof(line),
						    " %s  %.*s",
						    label,
						    name_len,
						    disp);
				}
				break;
			}

			len = strlen(line);
			if (len > 255) len = 255;
			w = TextWidth(line, 0, len) + 8;
			if (w > g_content_max_width)
				g_content_max_width = w;
		}
	} else if (g_page->page_type == PAGE_TEXT &&
	    g_page->text_buf && g_page->text_lines) {
		for (i = 0; i < total; i++) {
			const char *ls;
			short ll;

			ls = g_page->text_buf +
			    g_page->text_lines[i];
			if (i + 1 < total)
				ll = (short)(
				    g_page->text_lines[i + 1] -
				    g_page->text_lines[i] - 1);
			else {
				ll = (short)(g_page->text_len -
				    g_page->text_lines[i]);
				if (ll > 0 &&
				    ls[ll - 1] == '\r')
					ll--;
			}
			if (ll < 0) ll = 0;

#ifdef GEOMYS_CP437
			if (cp437_has_high(ls, ll)) {
				char xlated[256];
				short xlen = ll;

				if (xlen > 255) xlen = 255;
				xlen = cp437_translate(xlated,
				    ls, xlen);
				w = TextWidth(xlated, 0, xlen)
				    + 8;
			} else
#endif
				w = TextWidth((Ptr)ls, 0, ll)
				    + 8;
			if (w > g_content_max_width)
				g_content_max_width = w;
		}
	}

	SetPort(save);
}

/*
 * content_update_hscroll - update horizontal scrollbar range
 * based on content width vs visible width.
 */
static void
content_update_hscroll(WindowPtr win)
{
	Rect r;
	short visible_w, max_val;

	if (!g_hscrollbar)
		return;

	content_get_rect(win, &r);
	visible_w = r.right - r.left;

	max_val = g_content_max_width - visible_w;
	if (max_val < 0)
		max_val = 0;

	SetControlMaximum(g_hscrollbar, max_val);
	SetControlValue(g_hscrollbar, g_hscroll_pos);

	/* Dim if nothing to scroll */
	HiliteControl(g_hscrollbar,
	    max_val > 0 ? 0 : 255);
}

/*
 * Horizontal scroll bar action proc — pixel-based scrolling.
 */
static pascal void
hscroll_action(ControlHandle ctl, short part)
{
	short new_pos, max_val, char_w;
	WindowPtr win;
	GrafPtr save;
	Rect r;

	new_pos = g_hscroll_pos;
	max_val = GetControlMaximum(ctl);
	win = (*ctl)->contrlOwner;

	TextFont(g_font_id);
	TextSize(g_font_size);
	char_w = CharWidth('M');

	content_get_rect(win, &r);

	switch (part) {
	case inUpButton:
		new_pos -= char_w;
		break;
	case inDownButton:
		new_pos += char_w;
		break;
	case inPageUp:
		new_pos -= (r.right - r.left);
		break;
	case inPageDown:
		new_pos += (r.right - r.left);
		break;
	default:
		return;
	}

	if (new_pos < 0)
		new_pos = 0;
	if (new_pos > max_val)
		new_pos = max_val;

	if (new_pos == g_hscroll_pos)
		return;

	g_hscroll_pos = new_pos;
	SetControlValue(ctl, new_pos);

	GetPort(&save);
	SetPort(win);

	/* Full redraw for horizontal scroll */
	g_scrolling = 1;
	content_draw(win);
	g_scrolling = 0;

	SetPort(save);
}

/*
 * content_hscroll_step - return one character-width scroll step
 * in pixels, with the correct font set.
 */
short
content_hscroll_step(void)
{
	TextFont(g_font_id);
	TextSize(g_font_size);
	return CharWidth('M');
}

/*
 * content_hscroll_by - scroll horizontally by delta pixels.
 * Positive = right, negative = left. Clamps to [0, max].
 */
void
content_hscroll_by(short delta)
{
	short new_pos, max_val;
	WindowPtr win;
	GrafPtr save;

	if (!g_hscrollbar)
		return;

	max_val = GetControlMaximum(g_hscrollbar);
	new_pos = g_hscroll_pos + delta;

	if (new_pos < 0)
		new_pos = 0;
	if (new_pos > max_val)
		new_pos = max_val;

	if (new_pos == g_hscroll_pos)
		return;

	g_hscroll_pos = new_pos;
	SetControlValue(g_hscrollbar, new_pos);

	win = (*g_hscrollbar)->contrlOwner;
	GetPort(&save);
	SetPort(win);
	g_scrolling = 1;
	content_draw(win);
	g_scrolling = 0;
	SetPort(save);
}

/*
 * content_hscroll_to - scroll to absolute horizontal pixel
 * position. Clamps to [0, max].
 */
void
content_hscroll_to(short pos)
{
	short max_val;
	WindowPtr win;
	GrafPtr save;

	if (!g_hscrollbar)
		return;

	max_val = GetControlMaximum(g_hscrollbar);

	if (pos < 0)
		pos = 0;
	if (pos > max_val)
		pos = max_val;

	if (pos == g_hscroll_pos)
		return;

	g_hscroll_pos = pos;
	SetControlValue(g_hscrollbar, pos);

	win = (*g_hscrollbar)->contrlOwner;
	GetPort(&save);
	SetPort(win);
	g_scrolling = 1;
	content_draw(win);
	g_scrolling = 0;
	SetPort(save);
}

/*
 * content_hscroll_click - handle a click on the horizontal
 * scrollbar. Called from main event loop.
 */
void
content_hscroll_click(WindowPtr win, Point local_pt, short part)
{
	if (!g_hscrollbar)
		return;

	if (part == inThumb) {
		GrafPtr save;

		TrackControl(g_hscrollbar, local_pt, 0L);
		g_hscroll_pos =
		    GetControlValue(g_hscrollbar);

		/* Redraw after thumb release */
		GetPort(&save);
		SetPort(win);
		content_draw(win);
		SetPort(save);
	} else {
		TrackControl(g_hscrollbar, local_pt,
		    g_hscroll_upp);
	}
}

/*
 * Save/restore content area statics to/from session struct.
 * Used during session switching (GEOMYS_MAX_WINDOWS > 1).
 */
void
content_save_state(struct BrowserSession *s)
{
	s->scrollbar = g_scrollbar;
	s->hscrollbar = g_hscrollbar;
	s->scroll_pos = g_scroll_pos;
	s->hscroll_pos = g_hscroll_pos;
	s->content_max_width = g_content_max_width;
	s->hover_row = g_hover_row;
	s->row_height = g_row_height;
	s->font_id = g_font_id;
	s->font_size = g_font_size;
#ifdef GEOMYS_CLIPBOARD
	s->sel = g_sel;
	s->win_active = g_win_active;
#endif
}

void
content_load_state(struct BrowserSession *s)
{
	g_scrollbar = s->scrollbar;
	g_hscrollbar = s->hscrollbar;
	g_scroll_pos = s->scroll_pos;
	g_hscroll_pos = s->hscroll_pos;
	g_content_max_width = s->content_max_width;
	g_page = &s->gopher;
	g_hover_row = s->hover_row;
	g_row_height = s->row_height;
	g_font_id = s->font_id;
	g_font_size = s->font_size;
#ifdef GEOMYS_CLIPBOARD
	g_sel = s->sel;
	g_win_active = s->win_active;
#endif
}
