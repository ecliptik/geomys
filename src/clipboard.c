/*
 * clipboard.c - Clipboard and text selection for Geomys
 */

#ifdef GEOMYS_CLIPBOARD

#include <Quickdraw.h>
#include <Windows.h>
#include <Memory.h>
#include <Multiverse.h>
#include <string.h>

#include "clipboard.h"
#include "content.h"
#include "browser.h"
#include "gopher.h"
#include "main.h"

/* External gopher state from main.c */
extern GopherState g_gopher;

/*
 * clipboard_copy_content - extract selected rows as TEXT
 * and place on system clipboard via Scrap Manager.
 *
 * Directory pages: copies item->display for each selected row.
 * Text pages: copies raw text_buf lines via text_lines[] index.
 * Lines separated by \r (Mac convention).
 */
void
clipboard_copy_content(WindowPtr win)
{
	short sr, sc, er, ec, row;
	Handle h;
	long len;

	(void)win;

	if (!content_get_selection(&sr, &sc, &er, &ec))
		return;

	h = NewHandle(0);
	if (!h)
		return;

	len = 0;

	for (row = sr; row <= er; row++) {
		char buf[256];
		short tlen, copy_start, copy_len;
		char *p;

		tlen = content_row_text(row, buf,
		    sizeof(buf));

		/* Determine character range for this row */
		if (sr == er) {
			/* Single-row selection */
			copy_start = sc;
			copy_len = ec - sc;
		} else if (row == sr) {
			/* First row: from start_col to EOL */
			copy_start = sc;
			copy_len = tlen - sc;
		} else if (row == er) {
			/* Last row: from BOL to end_col */
			copy_start = 0;
			copy_len = ec;
		} else {
			/* Middle row: full line */
			copy_start = 0;
			copy_len = tlen;
		}

		/* Clamp */
		if (copy_start > tlen)
			copy_start = tlen;
		if (copy_start + copy_len > tlen)
			copy_len = tlen - copy_start;
		if (copy_len < 0)
			copy_len = 0;

		/* Grow handle for this chunk + \r */
		SetHandleSize(h, len + copy_len + 1);
		if (MemError() != noErr)
			break;

		HLock(h);
		p = *h + len;
		if (copy_len > 0)
			memcpy(p, buf + copy_start, copy_len);
		p[copy_len] = '\r';
		HUnlock(h);
		len += copy_len + 1;
	}

	/* Strip trailing \r */
	if (len > 0)
		len--;

	if (len > 0) {
		HLock(h);
		ZeroScrap();
		PutScrap(len, 'TEXT', *h);
		HUnlock(h);
	}

	DisposeHandle(h);
}

/*
 * clipboard_select_all - select all rows in content area
 */
void
clipboard_select_all(WindowPtr win)
{
	content_select_all(win);
}

#endif /* GEOMYS_CLIPBOARD */
