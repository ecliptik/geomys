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
#include <ctype.h>

#include "content.h"
#include "browser.h"
#include "gopher.h"
#include "gopher_types.h"
#include "main.h"
#include "settings.h"
#include "session.h"
#include "savefile.h"
#include "theme.h"
#include "color.h"
#include "gopher_icons.h"
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
static short g_cell_width = 6;      /* cached CharWidth('M') */
static short g_cell_baseline = 9;   /* cached font ascent */
static CursHandle g_hand_cursor = 0L;  /* hand cursor for links */
#ifdef GEOMYS_CLIPBOARD
static CursHandle g_ibeam_cursor = 0L; /* I-beam cursor for text */
#endif
static short g_hover_row = -1;         /* currently highlighted row, -1 = none */
static short g_selected_row = -1;      /* keyboard-selected row, -1 = none */
static short g_in_full_draw = 0;       /* 1 during content_draw loop — skip per-row clip/restore */
static RgnHandle g_clip_rgn = 0L;     /* pre-allocated clip save/restore region */

/* Dirty-row tracking: skip redrawing rows that haven't changed */
static unsigned char g_dirty[512];     /* per-row dirty flag */
static short g_dirty_all = 1;          /* 1 = redraw all rows */
static short g_dirty_count = 0;        /* number of individually dirty rows */

/* Shadow buffer: per-row state for change detection */
#define SHADOW_MAX_ROWS  64   /* max visible rows tracked */
typedef struct {
	short item_index;     /* absolute row index drawn here */
	short hover_flag;     /* was this row hovered? */
	short sel_start;      /* selection start col (-1 = none) */
	short sel_end;        /* selection end col */
} ShadowRow;
static ShadowRow g_shadow[SHADOW_MAX_ROWS];
static short g_shadow_valid;  /* 1 if shadow state is populated */

/*
 * shadow_needs_draw - Check if a row's state has changed since last draw.
 * Returns 1 if the row needs redrawing, 0 if shadow matches.
 * sel_cached: 1 if normalized selection bounds are valid.
 */
static short
shadow_needs_draw(short slot, short row_index, short hover_flag,
    short sel_cached, short norm_sr, short norm_sc,
    short norm_er, short norm_ec)
{
	ShadowRow cur;

	if (!g_shadow_valid || slot >= SHADOW_MAX_ROWS)
		return 1;

	cur.item_index = row_index;
	cur.hover_flag = hover_flag;
	cur.sel_start = -1;
	cur.sel_end = -1;

	if (sel_cached && row_index >= norm_sr &&
	    row_index <= norm_er) {
		cur.sel_start = (row_index == norm_sr) ?
		    norm_sc : 0;
		cur.sel_end = (row_index == norm_er) ?
		    norm_ec : 32767;
	}

	if (cur.item_index == g_shadow[slot].item_index &&
	    cur.hover_flag == g_shadow[slot].hover_flag &&
	    cur.sel_start == g_shadow[slot].sel_start &&
	    cur.sel_end == g_shadow[slot].sel_end)
		return 0;

	return 1;
}

/*
 * shadow_update - Record current row state in shadow buffer.
 */
static void
shadow_update(short slot, short row_index, short hover_flag,
    short sel_cached, short norm_sr, short norm_sc,
    short norm_er, short norm_ec)
{
	if (slot >= SHADOW_MAX_ROWS)
		return;

	g_shadow[slot].item_index = row_index;
	g_shadow[slot].hover_flag = hover_flag;
	g_shadow[slot].sel_start = -1;
	g_shadow[slot].sel_end = -1;

	if (sel_cached && row_index >= norm_sr &&
	    row_index <= norm_er) {
		g_shadow[slot].sel_start =
		    (row_index == norm_sr) ? norm_sc : 0;
		g_shadow[slot].sel_end =
		    (row_index == norm_er) ? norm_ec : 32767;
	}
}

#ifdef GEOMYS_CLIPBOARD
/* Selection state */
static Selection g_sel;
static short g_win_active = 1;  /* window active flag for selection dimming */
static char g_sel_text_buf[256]; /* temp buffer for selection redraw */

/* Find in page state */
static char g_find_query[64];     /* last search term */
static short g_find_last_row;     /* row of last match (-1 = none) */
static short g_find_active;       /* 1 = have an active search */

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
	if (g_page->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || g_page->page_type == PAGE_HTML
#endif
	    )
		return g_page->text_line_count;
	return 0;
}

void
content_mark_dirty(short row)
{
	if (row >= 0 && row < 512) {
		if (!g_dirty[row])
			g_dirty_count++;
		g_dirty[row] = 1;
	}
}

void
content_mark_all_dirty(void)
{
	g_dirty_all = 1;
	g_dirty_count = 0;
	g_shadow_valid = 0;
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

	/* Pre-allocate reusable clip save/restore region.
	 * Avoids per-row NewRgn/DisposeRgn overhead. */
	g_clip_rgn = NewRgn();

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
	if (g_clip_rgn) {
		DisposeRgn(g_clip_rgn);
		g_clip_rgn = 0L;
	}
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
	content_mark_all_dirty();
	g_shadow_valid = 0;
#ifdef GEOMYS_CLIPBOARD
	/* Clear selection on page change */
	g_sel.active = 0;
	g_sel.selecting = 0;
	g_sel.word_mode = 0;
	g_sel.last_click_ticks = 0;
	/* Reset find state on page change */
	g_find_active = 0;
	g_find_last_row = -1;
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

/*
 * draw_selection_rect - highlight a selection region using
 * theme sel_bg/sel_fg on color systems, or InvertRect on mono.
 * Fills the rect with sel_bg, then redraws the row text clipped
 * to the selection rect using sel_fg for true inverse theming.
 */
#ifdef GEOMYS_CLIPBOARD
static void
draw_selection_rect(Rect *inv_r, short row_index,
    short y, WindowPtr win,
    const char *row_text, short row_text_len)
{
#ifdef GEOMYS_THEMES
#ifdef GEOMYS_COLOR
	if (g_has_color_qd) {
		const ThemeColors *st = theme_current();
		if (st) {
			Rect cr;
			const char *text;
			short text_len;
			short text_x;

			/* Fill selection background */
			theme_set_bg(&st->sel_bg);
			EraseRect(inv_r);
			theme_set_fg(&st->sel_fg);

			/* Use pre-formatted text if provided,
			 * otherwise fetch it */
			if (row_text && row_text_len > 0) {
				text = row_text;
				text_len = row_text_len;
			} else {
				char row_buf[256];

				text_len = content_row_text(
				    row_index, row_buf,
				    sizeof(row_buf));
				if (text_len > 0) {
					memcpy(g_sel_text_buf,
					    row_buf, text_len);
					g_sel_text_buf[text_len]
					    = '\0';
					text = g_sel_text_buf;
				} else {
					text = 0L;
				}
			}

			/* Redraw text clipped to selection.
			 * During full draw, content_draw() owns
			 * g_clip_rgn — use content rect to
			 * restore clip instead of the region.
			 * Outside full draw, save/restore via
			 * g_clip_rgn as usual. */
			if (text && text_len > 0) {
				content_get_rect(win, &cr);
				if (!g_in_full_draw && g_clip_rgn)
					GetClip(g_clip_rgn);
				ClipRect(inv_r);

				text_x = cr.left + 4 -
				    g_hscroll_pos;
				MoveTo(text_x, y - 2);
				DrawText((Ptr)text, 0, text_len);

				if (g_in_full_draw)
					ClipRect(&cr);
				else if (g_clip_rgn)
					SetClip(g_clip_rgn);
			}

			theme_reset_cache();
			return;
		}
	}
#endif
#endif
	/* Mono fallback: plain inversion */
	InvertRect(inv_r);
}
#endif /* GEOMYS_CLIPBOARD */

/*
 * format_row_text - Build formatted row string for a directory item.
 * Produces the same output as the old snprintf patterns:
 *   "      %.*s"  (icons style or info items)
 *   " DIR  %.*s"  (text style, normal)
 *   " DIR+ %.*s"  (text style, Gopher+)
 *   " <DL> %.*s"  (text style, download)
 *   " <DL+> %.*s" (text style, download with Gopher+)
 * Uses memcpy for the fixed prefixes — avoids snprintf overhead on 68000.
 * Returns length of text placed in buf.
 * If out_split is non-NULL, stores the split position in display text
 * (first double-space separator), or -1 if none found.
 */
static short
format_row_text(GopherItem *item, short page_style, char *buf,
    short bufsiz, short *out_split)
{
	const char *disp = item->display;
	short dlen = strlen(disp);
	short split_pos = -1;
	short name_len, li, pos, lbl_len, avail;
	const char *label;

	/* Find split point: first run of 2+ spaces separates
	 * name from metadata (skip info lines — plain text may
	 * have natural double spaces e.g. after periods) */
	if (item->type != GOPHER_INFO) {
		for (li = 1; li < dlen - 1; li++) {
			if (disp[li] == ' ' &&
			    disp[li + 1] == ' ') {
				split_pos = li;
				break;
			}
		}
	}

	if (out_split)
		*out_split = split_pos;

	name_len = (split_pos > 0) ? split_pos : dlen;

	if (bufsiz <= 0)
		return 0;

	pos = 0;

	if (page_style == STYLE_ICONS ||
	    item->type == GOPHER_INFO) {
		/* "      %.*s" — 6 spaces + name */
		if (pos + 6 < bufsiz) {
			memcpy(buf + pos, "      ", 6);
			pos += 6;
		}
	} else {
		label = gopher_type_label(item->type);
		lbl_len = strlen(label);

		if (gopher_type_is_download(item->type)) {
#ifdef GEOMYS_GOPHER_PLUS
			if (item->has_plus) {
				/* " <%s+> %.*s" */
				if (pos + 2 < bufsiz) {
					buf[pos++] = ' ';
					buf[pos++] = '<';
				}
				avail = bufsiz - pos - 4;
				if (lbl_len > avail)
					lbl_len = avail;
				if (lbl_len > 0) {
					memcpy(buf + pos, label,
					    lbl_len);
					pos += lbl_len;
				}
				if (pos + 3 < bufsiz) {
					buf[pos++] = '+';
					buf[pos++] = '>';
					buf[pos++] = ' ';
				}
			} else
#endif
			{
				/* " <%s> %.*s" */
				if (pos + 2 < bufsiz) {
					buf[pos++] = ' ';
					buf[pos++] = '<';
				}
				avail = bufsiz - pos - 3;
				if (lbl_len > avail)
					lbl_len = avail;
				if (lbl_len > 0) {
					memcpy(buf + pos, label,
					    lbl_len);
					pos += lbl_len;
				}
				if (pos + 2 < bufsiz) {
					buf[pos++] = '>';
					buf[pos++] = ' ';
				}
			}
		} else {
#ifdef GEOMYS_GOPHER_PLUS
			if (item->has_plus) {
				/* " %s+ %.*s" */
				if (pos + 1 < bufsiz)
					buf[pos++] = ' ';
				avail = bufsiz - pos - 3;
				if (lbl_len > avail)
					lbl_len = avail;
				if (lbl_len > 0) {
					memcpy(buf + pos, label,
					    lbl_len);
					pos += lbl_len;
				}
				if (pos + 2 < bufsiz) {
					buf[pos++] = '+';
					buf[pos++] = ' ';
				}
			} else
#endif
			{
				/* " %s  %.*s" */
				if (pos + 1 < bufsiz)
					buf[pos++] = ' ';
				avail = bufsiz - pos - 3;
				if (lbl_len > avail)
					lbl_len = avail;
				if (lbl_len > 0) {
					memcpy(buf + pos, label,
					    lbl_len);
					pos += lbl_len;
				}
				if (pos + 2 < bufsiz) {
					buf[pos++] = ' ';
					buf[pos++] = ' ';
				}
			}
		}
	}

	/* Append name portion */
	avail = bufsiz - pos - 1;  /* leave room for NUL */
	if (name_len > avail)
		name_len = avail;
	if (name_len > 0) {
		memcpy(buf + pos, disp, name_len);
		pos += name_len;
	}

	buf[pos] = '\0';
	return pos;
}

#if defined(GEOMYS_THEMES) && defined(GEOMYS_COLOR)
/*
 * set_item_fg_color - Set QuickDraw foreground color based on item type.
 * Extracts the repeated type-to-color selection pattern.
 */
static void
set_item_fg_color(const ThemeColors *t, char type)
{
	if (type == GOPHER_ERROR)
		theme_set_fg(&t->link_error);
	else if (type == GOPHER_SEARCH)
		theme_set_fg(&t->link_search);
	else if (gopher_type_is_download(type))
		theme_set_fg(&t->link_download);
	else if (gopher_type_navigable(type) ||
	    type == GOPHER_HTML)
		theme_set_fg(&t->link);
	else
		theme_set_fg(&t->link_external);
}
#endif

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
	short len, text_width, split_pos;
	Str255 ps;
	extern GeomysPrefs g_prefs;
#ifdef GEOMYS_THEMES
	const ThemeColors *t;
#endif

	if (!g_page || g_page->page_type != PAGE_DIRECTORY)
		return;
	if (row_index < 0 || row_index >= g_page->item_count)
		return;

	content_get_rect(win, &r);

	/* Skip if not visible */
	if (row_index < g_scroll_pos ||
	    row_index > g_scroll_pos + visible_rows(win))
		return;

	if (!g_in_full_draw && g_clip_rgn) {
		GetClip(g_clip_rgn);
		ClipRect(&r);
	}

	y = r.top + (row_index - g_scroll_pos + 1)
	    * g_row_height;

	SetRect(&erase_r, r.left, y - g_row_height,
	    r.right, y);

#ifdef GEOMYS_THEMES
	t = theme_current();
	{
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
					else
						set_item_fg_color(t,
						    ti->type);
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

	/* Format line with style prefix (memcpy-based) */
	len = format_row_text(item, g_prefs.page_style, line,
	    sizeof(line), &split_pos);
	if (len > 255) len = 255;

	/* Measure name width */
	text_width = TextWidth(line, 0, len);

	/* Draw with horizontal offset —
	 * ClipRect handles clipping automatically.
	 * Traditional style: draw label prefix in
	 * label color, then name in link color. */
#if defined(GEOMYS_THEMES) && defined(GEOMYS_COLOR)
	if (g_has_color_qd &&
	    item->type != GOPHER_INFO) {
		if (t && g_prefs.page_style ==
		    STYLE_TEXT) {
			/* Text style: label prefix in
			 * label color, name in link
			 * color */
			short lbl_len = 5;
			Str255 lps;

#ifdef GEOMYS_GOPHER_PLUS
			if (item->has_plus)
				lbl_len = 5;
#endif
			if (lbl_len > len)
				lbl_len = len;

			/* Draw label in label color */
			theme_set_fg(&t->label);
			lps[0] = lbl_len;
			memcpy(lps + 1, line,
			    lbl_len);
			MoveTo(r.left + 4 -
			    g_hscroll_pos, y - 2);
			DrawString(lps);

			/* Draw name in link color */
			set_item_fg_color(t, item->type);
			if (len > lbl_len) {
				ps[0] = len - lbl_len;
				memcpy(ps + 1,
				    line + lbl_len,
				    len - lbl_len);
				DrawString(ps);
			}
		} else if (t) {
			/* Icons style: all text in
			 * link color */
			set_item_fg_color(t, item->type);
			ps[0] = len;
			memcpy(ps + 1, line, len);
			MoveTo(r.left + 4 -
			    g_hscroll_pos, y - 2);
			DrawString(ps);
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

	/* Draw icon for Icons style */
	if (g_prefs.page_style == STYLE_ICONS &&
	    item->type != GOPHER_INFO) {
		const GopherIcon *gi;
		gi = gopher_icon_for_type(
		    item->type);
		if (gi) {
			short ix, iy, inv;
			ix = r.left + 4 -
			    g_hscroll_pos;
			iy = y - g_row_height +
			    (g_row_height -
			    gi->height) / 2;
			inv = 0;
#ifdef GEOMYS_THEMES
			/* Mono dark: use srcBic to
			 * draw white icons on black
			 * background */
			if (theme_is_dark() &&
			    !theme_is_color())
				inv = 1;
#if defined(GEOMYS_COLOR)
			if (g_has_color_qd && t) {
				theme_set_fg(
				    &t->label);
				inv = 0;
			}
#endif
#endif
			gopher_icon_draw(gi,
			    ix, iy, inv);
		}
	}

	/* Draw metadata right-aligned when Show
	 * Details is on and metadata exists.
	 * Metadata stays fixed at right edge
	 * (not affected by hscroll). */
	if (split_pos > 0 && g_prefs.show_details) {
		const char *disp = item->display;
		short dlen = strlen(disp);
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
#ifdef GEOMYS_COLOR
				if (t && g_has_color_qd)
					theme_set_fg(
					    &t->metadata);
#endif
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

	/* Underline on hover */
	{
		short show_underline;

		show_underline = (row_index == g_hover_row);

		if (show_underline) {
			/* line now contains only prefix + name
			 * (no metadata), so underline it all */
#ifdef GEOMYS_THEMES
			if (t && t->is_dark && !theme_is_color()) {
				/* Mono dark: pen is black on black bg.
				 * Use patBic to draw white underline
				 * matching the srcBic text. */
				PenMode(patBic);
			}
#endif
			MoveTo(r.left + 4 - g_hscroll_pos,
			    y - 1);
			LineTo(r.left + 4 - g_hscroll_pos +
			    TextWidth(line, 0, len), y - 1);
#ifdef GEOMYS_THEMES
			if (t && t->is_dark && !theme_is_color())
				PenNormal();
#endif
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
				draw_selection_rect(
				    &inv_r, row_index,
				    y, win, line, len);
			}
		}
	}
#endif

	/* Keyboard selection focus ring */
	if (row_index == g_selected_row) {
		PenState pen_save;
		Rect ring_r;

		GetPenState(&pen_save);
		PenSize(1, 1);
		PenPat(&qd.gray);
		SetRect(&ring_r, r.left + 1, y - g_row_height,
		    r.right - 1, y);
		FrameRect(&ring_r);
		SetPenState(&pen_save);
	}

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

	if (!g_in_full_draw && g_clip_rgn)
		SetClip(g_clip_rgn);
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

	if (!g_page || (g_page->page_type != PAGE_TEXT
#ifdef GEOMYS_HTML
	    && g_page->page_type != PAGE_HTML
#endif
	    ))
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

	if (!g_in_full_draw && g_clip_rgn) {
		GetClip(g_clip_rgn);
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
				draw_selection_rect(
				    &inv_r, line_index,
				    y, win, 0L, 0);
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

	if (!g_in_full_draw && g_clip_rgn)
		SetClip(g_clip_rgn);
}

void
content_draw(WindowPtr win)
{
	Rect r, erase_r;
	short i, start_row, end_row;
	short last_y;
#ifdef GEOMYS_CLIPBOARD
	short norm_sr = -1, norm_sc = -1;
	short norm_er = -1, norm_ec = -1;
	short sel_cached = 0;
#endif
#ifdef GEOMYS_OFFSCREEN
	short use_offscreen;
	short dirty_lo = 32767;  /* lowest drawn row (screen pos) */
	short dirty_hi = -1;     /* highest drawn row (screen pos) */
#endif

	/* If no explicit dirty marks, treat as full redraw.
	 * This ensures callers that don't mark dirty still
	 * get correct behavior (backward compatible). */
	if (!g_dirty_all) {
		if (g_dirty_count == 0) {
			g_dirty_all = 1;
			/* No explicit dirty marks — caller wants a
			 * full redraw.  Invalidate shadow to prevent
			 * stale slot data from skipping new rows
			 * (e.g. after page transition during load). */
			g_shadow_valid = 0;
		}
	}

	content_get_rect(win, &r);

#ifdef GEOMYS_THEMES
	theme_reset_cache();
#endif

#ifdef GEOMYS_OFFSCREEN
	use_offscreen = offscreen_is_ready();
	if (use_offscreen)
		offscreen_begin(win);
#endif

	/* Clip to content area (excluding scrollbar) */
	if (!g_clip_rgn)
		return;
	GetClip(g_clip_rgn);
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
		SetClip(g_clip_rgn);
		return;
	}

	start_row = g_scroll_pos;
	end_row = start_row + visible_rows(win) + 1;
	last_y = r.top;

#ifdef GEOMYS_CLIPBOARD
	/* Cache normalized selection once before the draw
	 * loop — avoids calling sel_normalize() per row */
	if (g_sel.active) {
		norm_sr = g_sel.anchor_row;
		norm_sc = g_sel.anchor_col;
		norm_er = g_sel.extent_row;
		norm_ec = g_sel.extent_col;
		sel_normalize(&norm_sr, &norm_sc,
		    &norm_er, &norm_ec);
		sel_cached = 1;
	}
#endif

	g_in_full_draw = 1;

	if (g_page->page_type == PAGE_DIRECTORY) {
		if (end_row > g_page->item_count)
			end_row = g_page->item_count;

		for (i = start_row; i < end_row; i++) {
			short slot = i - start_row;
			short need_draw;

			need_draw = g_dirty_all ||
			    (i < 512 && g_dirty[i]);

			if (need_draw &&
			    !shadow_needs_draw(slot, i,
			    (i == g_hover_row),
#ifdef GEOMYS_CLIPBOARD
			    sel_cached, norm_sr, norm_sc,
			    norm_er, norm_ec
#else
			    0, -1, -1, -1, -1
#endif
			    ))
				need_draw = 0;

			if (need_draw) {
				content_draw_row(win, i);
#ifdef GEOMYS_OFFSCREEN
				if (slot < dirty_lo)
					dirty_lo = slot;
				if (slot > dirty_hi)
					dirty_hi = slot;
#endif
				shadow_update(slot, i,
				    (i == g_hover_row),
#ifdef GEOMYS_CLIPBOARD
				    sel_cached, norm_sr,
				    norm_sc, norm_er,
				    norm_ec
#else
				    0, -1, -1, -1, -1
#endif
				    );
			}
		}

		if (end_row > start_row)
			last_y = r.top + (end_row - start_row)
			    * g_row_height;
	} else if (g_page->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || g_page->page_type == PAGE_HTML
#endif
	    ) {
		short text_end;

		if (!g_page->text_buf || !g_page->text_lines)
			goto done;

		text_end = g_page->text_line_count;
		if (end_row > text_end)
			end_row = text_end;

		for (i = start_row; i < end_row; i++) {
			short slot = i - start_row;
			short need_draw;

			need_draw = g_dirty_all ||
			    (i < 512 && g_dirty[i]);

			if (need_draw &&
			    !shadow_needs_draw(slot, i, 0,
#ifdef GEOMYS_CLIPBOARD
			    sel_cached, norm_sr, norm_sc,
			    norm_er, norm_ec
#else
			    0, -1, -1, -1, -1
#endif
			    ))
				need_draw = 0;

			if (need_draw) {
				content_draw_text_row(win, i);
#ifdef GEOMYS_OFFSCREEN
				if (slot < dirty_lo)
					dirty_lo = slot;
				if (slot > dirty_hi)
					dirty_hi = slot;
#endif
				shadow_update(slot, i, 0,
#ifdef GEOMYS_CLIPBOARD
				    sel_cached, norm_sr,
				    norm_sc, norm_er,
				    norm_ec
#else
				    0, -1, -1, -1, -1
#endif
				    );
			}
		}

		if (end_row > start_row)
			last_y = r.top + (end_row - start_row)
			    * g_row_height;
	}

	g_in_full_draw = 0;

	/* Erase empty area below last row */
	if (last_y < r.bottom) {
#ifdef GEOMYS_OFFSCREEN
		/* Tail erase extends the dirty region to
		 * cover the bottom — force full blit */
		dirty_hi = 32767;
#endif
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

	/* Clear dirty flags after drawing */
	if (g_dirty_all) {
		memset(g_dirty, 0, sizeof(g_dirty));
		g_dirty_all = 0;
	} else {
		short di;
		for (di = start_row; di < end_row && di < 512; di++)
			g_dirty[di] = 0;
	}
	g_dirty_count = 0;

	/* Mark shadow as valid after a complete draw pass */
	g_shadow_valid = 1;

	/* Restore port colors to black/white so themed colors
	 * don't leak into chrome (nav bar, address bar, buttons) */
	theme_restore_colors();

	SetClip(g_clip_rgn);

#ifdef GEOMYS_OFFSCREEN
	/* Blit offscreen content to screen.
	 * Partial blit: if <= 20 rows drawn, blit only
	 * the union rect of dirty rows. Otherwise full. */
	if (use_offscreen) {
		if (dirty_hi >= 0 &&
		    (dirty_hi - dirty_lo + 1) <= 20) {
			Rect partial;
			SetRect(&partial, r.left,
			    r.top + dirty_lo * g_row_height,
			    r.right,
			    r.top + (dirty_hi + 1) *
			    g_row_height);
			/* Clamp to content rect */
			if (partial.top < r.top)
				partial.top = r.top;
			if (partial.bottom > r.bottom)
				partial.bottom = r.bottom;
			offscreen_end(win, &partial);
		} else {
			offscreen_end(win, &r);
		}
	}
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

		if (row < 0 || row >= g_page->item_count)
			return 0;

		item = &g_page->items[row];
		return format_row_text(item,
		    g_prefs.page_style, buf, bufsiz,
		    0L);

	} else if (g_page->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || g_page->page_type == PAGE_HTML
#endif
	    ) {
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

/*
 * sel_row_pixel_range - compute pixel x1,x2 for a row's
 * selection columns. Returns the pixel range for the
 * selection on this row given the normalized start/end
 * columns and whether this is the first/last/middle row.
 */
static void
sel_row_pixel_range(short rr, short sel_sr, short sel_sc,
    short sel_er, short sel_ec, WindowPtr win,
    const Rect *cr, short *x1, short *x2)
{
	if (sel_sr == sel_er) {
		/* Single row: col range */
		*x1 = col_to_pixel(rr, sel_sc, win);
		*x2 = col_to_pixel(rr, sel_ec, win);
	} else if (rr == sel_sr) {
		/* First row: start_col to EOL */
		*x1 = col_to_pixel(rr, sel_sc, win);
		*x2 = cr->right;
	} else if (rr == sel_er) {
		/* Last row: BOL to end_col */
		*x1 = cr->left;
		*x2 = col_to_pixel(rr, sel_ec, win);
	} else {
		/* Middle row: full width */
		*x1 = cr->left;
		*x2 = cr->right;
	}
}

/*
 * invert_xor_delta - XOR the symmetric difference between
 * old and new selection pixel ranges on a single row.
 * Used on monochrome systems where InvertRect is self-
 * inverting. Avoids full row redraws entirely.
 *
 * Cases:
 *   1. Row was not selected, now is: invert new range
 *   2. Row was selected, now is not: invert old range
 *   3. Both selected, ranges differ: invert the deltas
 */
static void
invert_xor_delta(short old_x1, short old_x2,
    short new_x1, short new_x2, short y_top, short y_bot)
{
	Rect delta_r;

	if (old_x1 == 0 && old_x2 == 0) {
		/* Case 1: newly selected — invert all */
		if (new_x2 > new_x1) {
			SetRect(&delta_r, new_x1, y_top,
			    new_x2, y_bot);
			InvertRect(&delta_r);
		}
		return;
	}

	if (new_x1 == 0 && new_x2 == 0) {
		/* Case 2: deselected — un-invert all */
		if (old_x2 > old_x1) {
			SetRect(&delta_r, old_x1, y_top,
			    old_x2, y_bot);
			InvertRect(&delta_r);
		}
		return;
	}

	/* Case 3: both selected — XOR the differences.
	 * The old and new ranges overlap; invert only
	 * the non-overlapping parts (symmetric diff). */

	/* Left delta: if start moved */
	if (old_x1 != new_x1) {
		short lo = old_x1 < new_x1 ?
		    old_x1 : new_x1;
		short hi = old_x1 > new_x1 ?
		    old_x1 : new_x1;
		if (hi > lo) {
			SetRect(&delta_r, lo, y_top,
			    hi, y_bot);
			InvertRect(&delta_r);
		}
	}

	/* Right delta: if end moved */
	if (old_x2 != new_x2) {
		short lo = old_x2 < new_x2 ?
		    old_x2 : new_x2;
		short hi = old_x2 > new_x2 ?
		    old_x2 : new_x2;
		if (hi > lo) {
			SetRect(&delta_r, lo, y_top,
			    hi, y_bot);
			InvertRect(&delta_r);
		}
	}
}

/* Begin tracking selection drag. Redraws rows as selection
 * extends. Returns 1 if the mouse moved beyond slop
 * (i.e. user is dragging), 0 if released without moving
 * (i.e. a click). Tracks character-level positions.
 *
 * Uses XOR delta rendering on monochrome to avoid flash:
 * instead of erase-draw-invert per row, only InvertRect
 * the changed pixel regions. On color systems, falls back
 * to clipped row redraws with offscreen buffering. */
static short
track_content_drag(WindowPtr win, Point start_pt, short start_row)
{
	Point pt;
	short row, col, prev_row, prev_col;
	short total;
	short dragged = 0;
	short use_xor;
	Rect r;

	content_get_rect(win, &r);
	total = count_rows();
	prev_row = start_row;
	prev_col = g_sel.anchor_col;

	/* Determine if we can use the fast XOR path.
	 * XOR works on mono where InvertRect is used for
	 * selection. Color themes use sel_bg/sel_fg colors,
	 * so XOR would produce wrong colors. */
	use_xor = 1;
#ifdef GEOMYS_COLOR
	if (g_has_color_qd) {
		const ThemeColors *t = theme_current();
		if (t)
			use_xor = 0;
	}
#endif

	/* Save/restore clip using pre-allocated region */
	if (!g_clip_rgn)
		return 0;
	GetClip(g_clip_rgn);
	ClipRect(&r);

	TextFont(g_font_id);
	TextSize(g_font_size);

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

			/* Process only rows where selection
			 * state actually changed */
			lo = old_sr < new_sr ?
			    old_sr : new_sr;
			hi = old_er > new_er ?
			    old_er : new_er;

			for (rr = lo; rr <= hi; rr++) {
				short old_in, new_in;
				short osc, oec, nsc, nec;
				short y;

				/* Was this row in old sel? */
				old_in = (rr >= old_sr &&
				    rr <= old_er);
				/* Is it in new sel? */
				new_in = (rr >= new_sr &&
				    rr <= new_er);

				if (!old_in && !new_in)
					continue;

				/* Compute old col range */
				if (old_in) {
					osc = (rr == old_sr) ?
					    old_sc : 0;
					oec = (rr == old_er) ?
					    old_ec : 32767;
				} else {
					osc = 0; oec = 0;
				}

				/* Compute new col range */
				if (new_in) {
					nsc = (rr == new_sr) ?
					    new_sc : 0;
					nec = (rr == new_er) ?
					    new_ec : 32767;
				} else {
					nsc = 0; nec = 0;
				}

				/* Skip if identical */
				if (old_in == new_in &&
				    osc == nsc && oec == nec)
					continue;

				/* Skip rows not visible */
				if (rr < g_scroll_pos ||
				    rr > g_scroll_pos +
				    visible_rows(win))
					continue;

				y = r.top + (rr - g_scroll_pos
				    + 1) * g_row_height;

				if (use_xor) {
					/* Mono XOR delta path:
					 * invert only changed
					 * pixels — no erase,
					 * no redraw, no flash */
					short ox1 = 0, ox2 = 0;
					short nx1 = 0, nx2 = 0;

					if (old_in)
						sel_row_pixel_range(
						    rr, old_sr,
						    old_sc, old_er,
						    old_ec, win, &r,
						    &ox1, &ox2);
					if (new_in)
						sel_row_pixel_range(
						    rr, new_sr,
						    new_sc, new_er,
						    new_ec, win, &r,
						    &nx1, &nx2);

					invert_xor_delta(
					    ox1, ox2, nx1, nx2,
					    y - g_row_height, y);
				} else {
					/* Color path: full row
					 * redraw (themed colors
					 * require erase+draw) */
					if (g_page->page_type ==
					    PAGE_DIRECTORY)
						content_draw_row(
						    win, rr);
					else
						content_draw_text_row(
						    win, rr);
				}
			}
		}

		prev_row = row;
		prev_col = col;
	}

	SetClip(g_clip_rgn);

	/* Invalidate shadow so next content_draw() fully
	 * resyncs with the current selection state */
	g_shadow_valid = 0;

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

	/* HTML type — extract URL or fetch as text */
	if (item->type == GOPHER_HTML) {
		if (strncmp(item->selector, "URL:", 4) == 0) {
			do_html_url_dialog(item->selector + 4,
			    item->display);
			content_draw(win);
			return true;
		}
		if (strncmp(item->selector, "GET ", 4) == 0) {
			char url[300];
			snprintf(url, sizeof(url),
			    "http://%s:%d%s",
			    item->host, item->port,
			    item->selector + 4);
			do_html_url_dialog(url, item->display);
			content_draw(win);
			return true;
		}
		/* Bare HTML — fetch and render */
		{
			char uri[300];
			gopher_build_uri(uri, sizeof(uri),
			    item->host, item->port,
#ifdef GEOMYS_HTML
			    GOPHER_HTML,
#else
			    GOPHER_TEXT,
#endif
			    item->selector);
			do_navigate_url_titled(uri,
			    item->display[0] ?
			    item->display : item->host);
			return true;
		}
	}

#ifdef GEOMYS_TELNET
	/* Telnet types — show connection dialog */
	if (item->type == GOPHER_TELNET ||
	    item->type == GOPHER_TN3270) {
		do_telnet_dialog(item->type, item->display,
		    item->host, item->port,
		    item->selector);
		content_draw(win);
		return true;
	}
#endif

#ifdef GEOMYS_DOWNLOAD
	/* Download types — save to disk */
	if (item->type == GOPHER_BINHEX ||
	    item->type == GOPHER_DOS ||
	    item->type == GOPHER_UUENCODE ||
	    item->type == GOPHER_BINARY ||
	    item->type == GOPHER_DOC ||
	    item->type == GOPHER_SOUND ||
	    item->type == GOPHER_RTF) {
		do_download_file(item);
		return true;
	}

	/* Image types — save to disk with header sniffing */
	if (item->type == GOPHER_GIF ||
	    item->type == GOPHER_IMAGE ||
	    item->type == GOPHER_PNG) {
		do_image_save(item);
		return true;
	}
#endif

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
content_click_row(WindowPtr win, GopherState *gs, short row)
{
#ifdef GEOMYS_CLIPBOARD
	return do_directory_navigate(win, gs, row);
#else
	GopherItem *item;

	if (!gs || row < 0 || row >= gs->item_count)
		return false;
	item = &gs->items[row];
	if (item->type == GOPHER_INFO)
		return false;
	if (item->type == GOPHER_SEARCH) {
		do_search_dialog(item->display,
		    item->host, item->port, item->selector);
		content_draw(win);
		return true;
	}
	if (item->type == GOPHER_HTML) {
		if (strncmp(item->selector, "URL:", 4) == 0) {
			do_html_url_dialog(item->selector + 4,
			    item->display);
			content_draw(win);
			return true;
		}
		if (strncmp(item->selector, "GET ", 4) == 0) {
			char url[300];
			snprintf(url, sizeof(url),
			    "http://%s:%d%s",
			    item->host, item->port,
			    item->selector + 4);
			do_html_url_dialog(url, item->display);
			content_draw(win);
			return true;
		}
		{
			char uri[300];
			gopher_build_uri(uri, sizeof(uri),
			    item->host, item->port,
#ifdef GEOMYS_HTML
			    GOPHER_HTML,
#else
			    GOPHER_TEXT,
#endif
			    item->selector);
			do_navigate_url_titled(uri,
			    item->display[0] ?
			    item->display : item->host);
			return true;
		}
	}
#ifdef GEOMYS_TELNET
	/* Telnet types — show connection dialog */
	if (item->type == GOPHER_TELNET ||
	    item->type == GOPHER_TN3270) {
		do_telnet_dialog(item->type, item->display,
		    item->host, item->port,
		    item->selector);
		content_draw(win);
		return true;
	}
#endif
#ifdef GEOMYS_DOWNLOAD
	/* Download types — save to disk */
	if (item->type == GOPHER_BINHEX ||
	    item->type == GOPHER_DOS ||
	    item->type == GOPHER_UUENCODE ||
	    item->type == GOPHER_BINARY ||
	    item->type == GOPHER_DOC ||
	    item->type == GOPHER_SOUND ||
	    item->type == GOPHER_RTF) {
		do_download_file(item);
		return true;
	}
	if (item->type == GOPHER_GIF ||
	    item->type == GOPHER_IMAGE ||
	    item->type == GOPHER_PNG) {
		do_image_save(item);
		return true;
	}
#endif
	if (gopher_type_navigable(item->type)) {
		char uri[300];

		gopher_build_uri(uri, sizeof(uri),
		    item->host, item->port,
		    item->type, item->selector);
		do_navigate_url_titled(uri, item->host);
		return true;
	}
	do_type_message(item->type, item->display,
	    item->host, item->port);
	content_draw(win);
	return true;
#endif
}

Boolean
content_click(WindowPtr win, Point local_pt, GopherState *gs)
{
	Rect r;
	short clicked_row;
	short total;

	content_get_rect(win, &r);
	if (!PtInRect(local_pt, &r))
		return false;

	/* Clear keyboard selection on mouse click */
	if (g_selected_row >= 0)
		content_clear_kbd_selection(win);

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

		if (item->type == GOPHER_HTML) {
			if (strncmp(item->selector,
			    "URL:", 4) == 0) {
				do_html_url_dialog(
				    item->selector + 4,
				    item->display);
				content_draw(win);
				return true;
			}
			if (strncmp(item->selector,
			    "GET ", 4) == 0) {
				char url[300];
				snprintf(url, sizeof(url),
				    "http://%s:%d%s",
				    item->host, item->port,
				    item->selector + 4);
				do_html_url_dialog(url,
				    item->display);
				content_draw(win);
				return true;
			}
			{
				char uri[300];
				gopher_build_uri(uri,
				    sizeof(uri),
				    item->host,
				    item->port,
#ifdef GEOMYS_HTML
				    GOPHER_HTML,
#else
				    GOPHER_TEXT,
#endif
				    item->selector);
				do_navigate_url_titled(uri,
				    item->display[0] ?
				    item->display :
				    item->host);
				return true;
			}
		}

#ifdef GEOMYS_TELNET
		/* Telnet types — show connection dialog */
		if (item->type == GOPHER_TELNET ||
		    item->type == GOPHER_TN3270) {
			do_telnet_dialog(item->type,
			    item->display,
			    item->host, item->port,
			    item->selector);
			content_draw(win);
			return true;
		}
#endif

#ifdef GEOMYS_DOWNLOAD
		/* Download types — save to disk */
		if (item->type == GOPHER_BINHEX ||
		    item->type == GOPHER_DOS ||
		    item->type == GOPHER_UUENCODE ||
		    item->type == GOPHER_BINARY ||
		    item->type == GOPHER_DOC) {
			do_download_file(item);
			return true;
		}
		if (item->type == GOPHER_GIF ||
		    item->type == GOPHER_IMAGE ||
		    item->type == GOPHER_PNG) {
			do_image_save(item);
			return true;
		}
#endif

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
		content_mark_all_dirty();

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
	content_mark_all_dirty();
	SetControlValue(ctl, new_pos);

	GetPort(&save);
	SetPort(win);

	if ((delta == 1 || delta == -1) &&
	    g_page && (g_page->page_type == PAGE_DIRECTORY ||
	    g_page->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || g_page->page_type == PAGE_HTML
#endif
	    )) {
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
		/* Page/thumb scroll — full redraw with
		 * offscreen buffering for flicker-free
		 * rendering (partial CopyBits keeps it fast) */
		content_draw(win);
	}

	SetPort(save);
}

void
content_resize(WindowPtr win)
{
	Rect sb_rect, content_r;

	content_mark_all_dirty();
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
	g_selected_row = -1;
	content_mark_all_dirty();
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
	content_mark_all_dirty();

	win = (*g_scrollbar)->contrlOwner;
	GetPort(&save);
	SetPort(win);
	content_draw(win);
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
	content_mark_all_dirty();

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
		g_cell_width = CharWidth('M');
		g_cell_baseline = fi.ascent;
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

	/* Check if hovering over a clickable item */
	if (g_page && g_page->page_type == PAGE_DIRECTORY &&
	    g_page->item_count > 0) {
		row = g_scroll_pos +
		    (local_pt.v - r.top) / g_row_height;

		if (row >= 0 && row < g_page->item_count) {
			GopherItem *item = &g_page->items[row];

			if (item->type != GOPHER_INFO &&
			    (gopher_type_navigable(item->type) ||
			    item->type == GOPHER_HTML ||
			    item->type == GOPHER_TELNET ||
			    item->type == GOPHER_TN3270 ||
			    gopher_type_is_download(item->type)))
				new_hover = row;
		}
	}

	/* Update hover — redraw only affected rows */
	if (new_hover != g_hover_row) {
		short old_hover = g_hover_row;

		g_hover_row = new_hover;
		if (old_hover >= 0) {
			content_mark_dirty(old_hover);
			content_draw_row(win, old_hover);
		}
		if (new_hover >= 0) {
			content_mark_dirty(new_hover);
			content_draw_row(win, new_hover);
		}
	}

	/* Status bar hover hints — only update when row changes */
	{
		static short prev_status_row = -1;

		row = -1;
		if (g_page && g_page->page_type ==
		    PAGE_DIRECTORY && g_page->item_count > 0) {
			row = g_scroll_pos +
			    (local_pt.v - r.top) / g_row_height;
			if (row < 0 || row >= g_page->item_count)
				row = -1;
		}

		if (row != prev_status_row) {
			prev_status_row = row;
			if (row >= 0) {
				GopherItem *item =
				    &g_page->items[row];
				char hint[80];

				if (item->type == GOPHER_INFO) {
					browser_set_status("");
				} else if (
				    gopher_type_is_download(
				    item->type)) {
					const char *lbl =
					    gopher_type_label(
					    item->type);
					snprintf(hint,
					    sizeof(hint),
					    "%s file \xD0 "
					    "click to save "
					    "to disk", lbl);
					browser_set_status(hint);
				} else if (item->type ==
				    GOPHER_HTML &&
				    strncmp(item->selector,
				    "URL:", 4) == 0) {
					snprintf(hint,
					    sizeof(hint),
					    "%.76s",
					    item->selector + 4);
					browser_set_status(hint);
				} else if (item->type ==
				    GOPHER_TELNET ||
				    item->type ==
				    GOPHER_TN3270) {
					snprintf(hint,
					    sizeof(hint),
					    "Telnet: %s:%d",
					    item->host,
					    item->port);
					browser_set_status(hint);
				} else {
					char uri[300];

					gopher_build_uri(uri,
					    sizeof(uri),
					    item->host,
					    item->port,
					    item->type,
					    item->selector);
					browser_set_status(uri);
				}
			} else {
				browser_set_status("");
			}
			browser_draw_status(win);
		}
	}

	/* Update cursor */
	if (new_hover >= 0) {
		if (g_hand_cursor)
			SetCursor(*g_hand_cursor);
#ifdef GEOMYS_CLIPBOARD
	} else if (g_page && (g_page->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || g_page->page_type == PAGE_HTML
#endif
	    )) {
		/* I-beam over text pages */
		if (g_ibeam_cursor)
			SetCursor(*g_ibeam_cursor);
	} else if (g_page && g_page->page_type == PAGE_DIRECTORY) {
		/* I-beam over non-navigable items */
		row = g_scroll_pos +
		    (local_pt.v - r.top) / g_row_height;
		if (row >= 0 && row < g_page->item_count &&
		    !gopher_type_navigable(
		    g_page->items[row].type) &&
		    g_page->items[row].type != GOPHER_HTML &&
		    !gopher_type_is_download(
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
			content_mark_dirty(i);
			if (g_page->page_type == PAGE_DIRECTORY)
				content_draw_row(win, i);
			else if (g_page->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
			    || g_page->page_type == PAGE_HTML
#endif
			    )
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
	content_mark_all_dirty();

	if (win) {
		GrafPtr save;

		GetPort(&save);
		SetPort(win);
		content_draw(win);
		SetPort(save);
	}
}

/*
 * Case-insensitive substring search.
 * Returns pointer into haystack on match, NULL otherwise.
 */
static const char *
ci_strstr(const char *haystack, const char *needle)
{
	short nlen, i;

	nlen = strlen(needle);
	if (nlen == 0)
		return haystack;

	for (; *haystack; haystack++) {
		for (i = 0; i < nlen; i++) {
			if (tolower((unsigned char)haystack[i]) !=
			    tolower((unsigned char)needle[i]))
				break;
		}
		if (i == nlen)
			return haystack;
	}
	return 0L;
}

/*
 * content_find - search for query in page content.
 * Case-insensitive substring search starting from
 * g_find_last_row + 1, wrapping around.
 * On match: scrolls to row, highlights match, returns true.
 * On no match: beeps, shows status message, returns false.
 */
Boolean
content_find(const char *query)
{
	short total, start, i, row;
	char buf[256];
	const char *match;
	short qlen, col;

	if (!query || !query[0])
		return false;

	total = count_rows();
	if (total == 0)
		return false;

	/* Save query for Find Again */
	strncpy(g_find_query, query, sizeof(g_find_query) - 1);
	g_find_query[sizeof(g_find_query) - 1] = '\0';
	g_find_active = 1;

	/* Start searching from last match + 1, or top */
	start = (g_find_last_row >= 0) ?
	    g_find_last_row + 1 : 0;
	if (start >= total)
		start = 0;

	qlen = strlen(query);

	/* Search with wrap-around */
	for (i = 0; i < total; i++) {
		row = (start + i) % total;

		buf[0] = '\0';
		content_row_text(row, buf, sizeof(buf));

		match = ci_strstr(buf, query);
		if (match) {
			/* Found — record position */
			g_find_last_row = row;

			/* Scroll to make match visible, centered */
			{
				short vis = content_visible_rows();
				short target = row - vis / 2;
				if (target < 0)
					target = 0;
				content_set_scroll_pos(target);
			}

			/* Highlight match using selection */
			col = (short)(match - buf);
			g_sel.active = 1;
			g_sel.selecting = 0;
			g_sel.anchor_row = row;
			g_sel.anchor_col = col;
			g_sel.extent_row = row;
			g_sel.extent_col = col + qlen;
			g_sel.word_mode = 0;
			g_hover_row = -1;
			content_mark_all_dirty();

			/* Redraw */
			if (g_window) {
				GrafPtr save;
				GetPort(&save);
				SetPort(g_window);
				content_draw(g_window);
				SetPort(save);
			}

			browser_set_status("Found");
			return true;
		}
	}

	/* Not found */
	g_find_last_row = -1;
	SysBeep(5);
	browser_set_status("Not found");
	return false;
}

Boolean
content_find_again(void)
{
	if (!g_find_active || !g_find_query[0])
		return false;
	return content_find(g_find_query);
}

Boolean
content_find_active(void)
{
	return g_find_active != 0;
}

const char *
content_find_query(void)
{
	return g_find_query;
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
			case STYLE_ICONS:
				snprintf(line, sizeof(line),
				    "      %.*s",
				    name_len, disp);
				break;
			default: /* STYLE_TEXT */
				if (item->type == GOPHER_INFO)
					snprintf(line,
					    sizeof(line),
					    "      %.*s",
					    name_len, disp);
				else if (
				    gopher_type_is_download(
				    item->type)) {
#ifdef GEOMYS_GOPHER_PLUS
					if (item->has_plus)
						snprintf(line,
						    sizeof(line),
						    " <%s+> %.*s",
						    label,
						    name_len,
						    disp);
					else
#endif
						snprintf(line,
						    sizeof(line),
						    " <%s> %.*s",
						    label,
						    name_len,
						    disp);
				} else {
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
	} else if ((g_page->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || g_page->page_type == PAGE_HTML
#endif
	    ) && g_page->text_buf && g_page->text_lines) {
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
	content_mark_all_dirty();

	GetPort(&save);
	SetPort(win);

	/* Full redraw for horizontal scroll */
	content_draw(win);

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
	content_draw(win);
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
	content_mark_all_dirty();

	win = (*g_hscrollbar)->contrlOwner;
	GetPort(&save);
	SetPort(win);
	content_draw(win);
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
		content_mark_all_dirty();

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
 * Keyboard link navigation — find next/prev navigable row.
 * Skips GOPHER_INFO items. Returns new selected row or -1.
 */
static short
find_navigable_row(short from, short dir)
{
	short total, i;

	if (!g_page || g_page->page_type != PAGE_DIRECTORY)
		return -1;

	total = g_page->item_count;
	i = from + dir;
	while (i >= 0 && i < total) {
		if (g_page->items[i].type != GOPHER_INFO)
			return i;
		i += dir;
	}
	return -1;
}

short
content_get_selected_row(void)
{
	return g_selected_row;
}

void
content_clear_kbd_selection(WindowPtr win)
{
	short old = g_selected_row;

	if (old < 0)
		return;
	g_selected_row = -1;
	content_mark_dirty(old);
	if (win) {
		GrafPtr save;

		GetPort(&save);
		SetPort(win);
		content_draw_row(win, old);
		SetPort(save);
	}
}

short
content_select_next(WindowPtr win)
{
	short old = g_selected_row;
	short next;

	if (!g_page || g_page->page_type != PAGE_DIRECTORY)
		return -1;

	next = find_navigable_row(old < 0 ? -1 : old, 1);
	if (next < 0)
		return g_selected_row;

	g_selected_row = next;
	content_mark_dirty(next);
	if (old >= 0)
		content_mark_dirty(old);

	/* Auto-scroll to keep selection visible */
	if (next < g_scroll_pos)
		content_set_scroll_pos(next);
	else if (next >= g_scroll_pos + visible_rows(win))
		content_set_scroll_pos(next - visible_rows(win) + 1);
	else if (win) {
		GrafPtr save;

		GetPort(&save);
		SetPort(win);
		if (old >= 0)
			content_draw_row(win, old);
		content_draw_row(win, next);
		SetPort(save);
	}
	return next;
}

short
content_select_prev(WindowPtr win)
{
	short old = g_selected_row;
	short prev;

	if (!g_page || g_page->page_type != PAGE_DIRECTORY)
		return -1;

	if (old < 0) {
		/* No selection — find last navigable row */
		prev = find_navigable_row(g_page->item_count, -1);
	} else {
		prev = find_navigable_row(old, -1);
	}
	if (prev < 0)
		return g_selected_row;

	g_selected_row = prev;
	content_mark_dirty(prev);
	if (old >= 0)
		content_mark_dirty(old);

	/* Auto-scroll to keep selection visible */
	if (prev < g_scroll_pos)
		content_set_scroll_pos(prev);
	else if (prev >= g_scroll_pos + visible_rows(win))
		content_set_scroll_pos(prev - visible_rows(win) + 1);
	else if (win) {
		GrafPtr save;

		GetPort(&save);
		SetPort(win);
		if (old >= 0)
			content_draw_row(win, old);
		content_draw_row(win, prev);
		SetPort(save);
	}
	return prev;
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
	s->selected_row = g_selected_row;
	s->row_height = g_row_height;
	s->font_id = g_font_id;
	s->font_size = g_font_size;
	s->cell_width = g_cell_width;
	s->cell_baseline = g_cell_baseline;
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
	g_selected_row = s->selected_row;
	g_row_height = s->row_height;
	g_font_id = s->font_id;
	g_font_size = s->font_size;
	g_cell_width = s->cell_width;
	g_cell_baseline = s->cell_baseline;
#ifdef GEOMYS_CLIPBOARD
	g_sel = s->sel;
	g_win_active = s->win_active;
#endif
}
