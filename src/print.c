/*
 * print.c - Printing support for Geomys
 */

#ifdef GEOMYS_PRINT

#include <Quickdraw.h>
#include <Memory.h>
#include <Fonts.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>

#include "print.h"
#include "main.h"
#include "gopher.h"
#include "gopher_types.h"
#include "macutil.h"
#include "settings.h"

extern GeomysPrefs g_prefs;

static THPrint g_print_rec = 0L;

static void
ensure_print_rec(void)
{
	if (g_print_rec)
		return;
	g_print_rec = (THPrint)NewHandle(sizeof(TPrint));
	if (g_print_rec)
		PrintDefault(g_print_rec);
}

void
do_page_setup(void)
{
	PrOpen();
	if (PrError() != noErr) {
		PrClose();
		show_error_alert(
		    "Could not open the Printing Manager.");
		return;
	}

	ensure_print_rec();
	if (!g_print_rec) {
		PrClose();
		show_error_alert(
		    "Not enough memory for printing.");
		return;
	}

	PrStlDialog(g_print_rec);
	PrClose();
}

static void
draw_header_footer(const char *title, const char *url,
    short page_num, Rect *rPage)
{
	char buf[120];
	Str255 ps;
	short save_font, save_size;

	save_font = qd.thePort->txFont;
	save_size = qd.thePort->txSize;
	TextFont(3);  /* Geneva */
	TextSize(9);

	/* Header: title on left, URL on right (truncated) */
	if (title[0]) {
		snprintf(buf, sizeof(buf), "%.60s", title);
		c2pstr(ps, buf);
		MoveTo(rPage->left, rPage->top - 4);
		DrawString(ps);
	}

	/* Footer: page number centered */
	snprintf(buf, sizeof(buf), "Page %d", page_num);
	c2pstr(ps, buf);
	{
		short tw = StringWidth(ps);
		short cx = (rPage->left + rPage->right) / 2;
		MoveTo(cx - tw / 2, rPage->bottom + 12);
	}
	DrawString(ps);

	TextFont(save_font);
	TextSize(save_size);
}

static void
print_text_page(GopherState *gs, TPPrPort prPort,
    const char *title, const char *url)
{
	Rect rPage;
	short row_height, rows_per_page;
	short line, page_num, row_on_page;
	short font_ascent;

	rPage = (*g_print_rec)->prInfo.rPage;

	/* Inset for margins */
	rPage.top += 16;
	rPage.bottom -= 16;

	TextFont(g_prefs.font_id);
	TextSize(g_prefs.font_size);

	{
		FontInfo fi;
		GetFontInfo(&fi);
		row_height = fi.ascent + fi.descent + fi.leading;
		font_ascent = fi.ascent;
	}
	rows_per_page = (rPage.bottom - rPage.top) / row_height;
	if (rows_per_page < 1)
		rows_per_page = 1;

	page_num = 1;
	row_on_page = 0;

	draw_header_footer(title, url, page_num, &rPage);

	for (line = 0; line < gs->text_line_count; line++) {
		long start, end, len;
		const char *text;

		start = gs->text_lines[line];
		if (line + 1 < gs->text_line_count)
			end = gs->text_lines[line + 1];
		else
			end = gs->text_len;

		text = gs->text_buf + start;
		len = end - start;

		/* Strip trailing CR/LF */
		while (len > 0 &&
		    (text[len - 1] == '\r' ||
		    text[len - 1] == '\n'))
			len--;

		MoveTo(rPage.left,
		    rPage.top + row_on_page * row_height +
		    font_ascent);
		if (len > 0)
			DrawText(text, 0, (short)len);

		row_on_page++;
		if (row_on_page >= rows_per_page &&
		    line + 1 < gs->text_line_count) {
			PrClosePage(prPort);
			PrOpenPage(prPort, 0L);
			page_num++;
			row_on_page = 0;
			TextFont(g_prefs.font_id);
			TextSize(g_prefs.font_size);
			draw_header_footer(title, url,
			    page_num, &rPage);
		}
	}
}

static void
print_directory_page(GopherState *gs, TPPrPort prPort,
    const char *title, const char *url)
{
	Rect rPage;
	short row_height, rows_per_page;
	short i, page_num, row_on_page;
	short font_ascent;

	rPage = (*g_print_rec)->prInfo.rPage;
	rPage.top += 16;
	rPage.bottom -= 16;

	TextFont(g_prefs.font_id);
	TextSize(g_prefs.font_size);

	{
		FontInfo fi;
		GetFontInfo(&fi);
		row_height = fi.ascent + fi.descent + fi.leading;
		font_ascent = fi.ascent;
	}
	rows_per_page = (rPage.bottom - rPage.top) / row_height;
	if (rows_per_page < 1)
		rows_per_page = 1;

	page_num = 1;
	row_on_page = 0;

	draw_header_footer(title, url, page_num, &rPage);

	for (i = 0; i < gs->item_count; i++) {
		GopherItem *item = &gs->items[i];
		char line_buf[120];
		short len;

		if (item->type == GOPHER_INFO) {
			len = strlen(item->display);
			if (len > (short)sizeof(line_buf) - 1)
				len = sizeof(line_buf) - 1;
			memcpy(line_buf, item->display, len);
			line_buf[len] = '\0';
		} else {
			const char *label;
			label = gopher_type_label(item->type);
			snprintf(line_buf, sizeof(line_buf),
			    "[%s]  %s", label, item->display);
		}

		MoveTo(rPage.left,
		    rPage.top + row_on_page * row_height +
		    font_ascent);
		DrawText(line_buf, 0, strlen(line_buf));

		row_on_page++;
		if (row_on_page >= rows_per_page &&
		    i + 1 < gs->item_count) {
			PrClosePage(prPort);
			PrOpenPage(prPort, 0L);
			page_num++;
			row_on_page = 0;
			TextFont(g_prefs.font_id);
			TextSize(g_prefs.font_size);
			draw_header_footer(title, url,
			    page_num, &rPage);
		}
	}
}

void
do_print(void)
{
	GopherState *gs = &g_gopher;
	TPPrPort prPort;
	TPrStatus prStatus;
	char uri[300];
	char title[80];

	if (gs->page_type == PAGE_NONE ||
	    g_app_state != APP_STATE_IDLE)
		return;

	PrOpen();
	if (PrError() != noErr) {
		PrClose();
		show_error_alert(
		    "Could not open the Printing Manager.");
		return;
	}

	ensure_print_rec();
	if (!g_print_rec) {
		PrClose();
		show_error_alert(
		    "Not enough memory for printing.");
		return;
	}

	if (!PrJobDialog(g_print_rec)) {
		PrClose();
		return;
	}

	/* Build title and URL for header */
	strncpy(title, gs->cur_title, sizeof(title) - 1);
	title[sizeof(title) - 1] = '\0';
	gopher_build_uri(uri, sizeof(uri),
	    gs->cur_host, gs->cur_port,
	    gs->cur_type, gs->cur_selector);

	prPort = PrOpenDoc(g_print_rec, 0L, 0L);
	if (PrError() != noErr) {
		PrCloseDoc(prPort);
		PrClose();
		show_error_alert("Error starting print job.");
		return;
	}

	PrOpenPage(prPort, 0L);
	if (PrError() == noErr) {
		if (gs->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
		    || gs->page_type == PAGE_HTML
#endif
		    )
			print_text_page(gs, prPort, title, uri);
		else
			print_directory_page(gs, prPort,
			    title, uri);
	}
	PrClosePage(prPort);
	PrCloseDoc(prPort);

	/* Check for spool printing (ImageWriter) */
	if (PrError() == noErr &&
	    (*g_print_rec)->prJob.bJDocLoop == bSpoolLoop)
		PrPicFile(g_print_rec, 0L, 0L, 0L, &prStatus);

	PrClose();
}

#endif /* GEOMYS_PRINT */
