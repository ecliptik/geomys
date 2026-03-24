/*
 * gopher.c - RFC 1436 Gopher protocol engine
 */

#include <Memory.h>
#include <Quickdraw.h>
#include <Events.h>
#include <Windows.h>
#include <Dialogs.h>
#include <ToolUtils.h>
#include <Files.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gopher.h"
#include "connection.h"
#include "dialogs.h"
#include "macutil.h"
#include "main.h"
#ifdef GEOMYS_DOWNLOAD
#include "imgparse.h"
#endif
#ifdef GEOMYS_HTML
#include "html.h"
#endif

/* Forward declarations */
static void gopher_parse_line(GopherState *gs, const char *line, short len);
static void gopher_process_data(GopherState *gs);
static void cso_process_data(GopherState *gs, char *buf, short len);

void
gopher_init(GopherState *gs)
{
	memset(gs, 0, sizeof(GopherState));
	gs->conn.dns_server = 0x01010101UL;  /* 1.1.1.1 default */
	gs->cso_last_entry = -1;
}

void
gopher_cleanup(GopherState *gs)
{
	if (gs->conn.state != CONN_STATE_IDLE)
		conn_close(&gs->conn);

	if (gs->items) {
		DisposePtr((Ptr)gs->items);
		gs->items = 0L;
	}
	if (gs->text_buf) {
		DisposePtr(gs->text_buf);
		gs->text_buf = 0L;
	}
	if (gs->text_lines) {
		DisposePtr((Ptr)gs->text_lines);
		gs->text_lines = 0L;
	}
}

/*
 * Free current page content, keeping connection state.
 */
static void
gopher_clear_page(GopherState *gs)
{
	if (gs->items) {
		DisposePtr((Ptr)gs->items);
		gs->items = 0L;
	}
	gs->item_count = 0;
	gs->item_capacity = 0;

	if (gs->text_buf) {
		DisposePtr(gs->text_buf);
		gs->text_buf = 0L;
	}
	gs->text_len = 0;

	if (gs->text_lines) {
		DisposePtr((Ptr)gs->text_lines);
		gs->text_lines = 0L;
	}
	gs->text_line_count = 0;

#ifdef GEOMYS_DOWNLOAD
	if (gs->dl_refnum) {
		FSClose(gs->dl_refnum);
		gs->dl_refnum = 0;
	}
	gs->dl_written = 0;
	gs->dl_error = false;
	gs->dl_vrefnum = 0;
	gs->dl_filename[0] = 0;
#endif

#ifdef GEOMYS_HTML
	html_init(gs);
#endif

	gs->page_type = PAGE_NONE;
	gs->line_len = 0;
}

Boolean
gopher_navigate(GopherState *gs, const char *host, short port,
    char type, const char *selector)
{
	Boolean ok;
	GopherItem *new_items = 0L;
	char *new_text = 0L;
	long *new_lines = 0L;
	short new_page_type;

	/* Close any existing connection */
	if (gs->conn.state != CONN_STATE_IDLE)
		conn_close(&gs->conn);

	/* Allocate new page buffers BEFORE clearing old page.
	 * If connection fails, old page content is preserved. */
	if (type == GOPHER_DIRECTORY || type == GOPHER_SEARCH ||
	    type == '\0') {
		new_page_type = PAGE_DIRECTORY;
		new_items = (GopherItem *)NewPtr(
		    (long)sizeof(GopherItem) * GOPHER_INIT_ITEMS);
		if (!new_items)
			return false;
	} else if (type == GOPHER_TEXT || type == GOPHER_ERROR
	    || type == GOPHER_CSO
#ifdef GEOMYS_HTML
	    || type == GOPHER_HTML
#endif
	    ) {
#ifdef GEOMYS_HTML
		new_page_type = (type == GOPHER_HTML) ?
		    PAGE_HTML : PAGE_TEXT;
#else
		new_page_type = PAGE_TEXT;
#endif
		new_text = NewPtr(GOPHER_TEXT_INIT_SIZE);
		if (!new_text)
			return false;
		new_lines = (long *)NewPtr(
		    (long)GOPHER_INIT_TEXT_LINES * sizeof(long));
		if (!new_lines) {
			DisposePtr(new_text);
			return false;
		}
		new_lines[0] = 0;  /* first line starts at offset 0 */
#ifdef GEOMYS_DOWNLOAD
	} else if (type == GOPHER_BINHEX || type == GOPHER_DOS ||
	    type == GOPHER_UUENCODE || type == GOPHER_BINARY ||
	    type == GOPHER_DOC || type == GOPHER_SOUND ||
	    type == GOPHER_RTF) {
		/* Download types: no buffer allocation — data goes
		 * straight to disk via FSWrite in process_data */
		new_page_type = PAGE_DOWNLOAD;
	} else if (type == GOPHER_GIF || type == GOPHER_IMAGE ||
	    type == GOPHER_PNG) {
		/* Image types: sniff header first, then stream to
		 * disk. File is already open from do_image_save. */
		new_page_type = PAGE_IMAGE;
#endif
	} else {
		new_page_type = PAGE_DIRECTORY;
		new_items = (GopherItem *)NewPtr(
		    (long)sizeof(GopherItem) * GOPHER_INIT_ITEMS);
		if (!new_items)
			return false;
	}

	/* Try to connect before committing to new page */
	ok = conn_connect(&gs->conn, host, port, 0L);
	if (!ok) {
		/* Connection failed — free new buffers, keep old page */
		if (new_items)
			DisposePtr((Ptr)new_items);
		if (new_text)
			DisposePtr(new_text);
		if (new_lines)
			DisposePtr((Ptr)new_lines);
		return false;
	}

	/* Connection succeeded — now clear old page and switch.
	 * For downloads/images, preserve old page data so the
	 * directory listing stays visible during the download. */
#ifdef GEOMYS_DOWNLOAD
	if (new_page_type == PAGE_DOWNLOAD ||
	    new_page_type == PAGE_IMAGE) {
		gs->dl_prev_page = gs->page_type;
		gs->page_type = new_page_type;
		gs->line_len = 0;
	} else
#endif
	{
		gopher_clear_page(gs);

		/* Zero new items buffer to prevent stale data if
		 * item_count is ever inconsistent */
		if (new_items)
			memset(new_items, 0,
			    (long)sizeof(GopherItem) *
			    GOPHER_INIT_ITEMS);

		gs->page_type = new_page_type;
		gs->items = new_items;
		gs->item_capacity = new_items ?
		    GOPHER_INIT_ITEMS : 0;
		gs->text_buf = new_text;
		gs->text_buf_capacity = new_text ?
		    GOPHER_TEXT_INIT_SIZE : 0;
		gs->text_lines = new_lines;
		gs->text_lines_capacity = new_lines ?
		    GOPHER_INIT_TEXT_LINES : 0;
		gs->text_line_count = new_lines ? 1 : 0;
	}

#ifdef GEOMYS_DOWNLOAD
	/* Reset image sniff state for PAGE_IMAGE */
	if (new_page_type == PAGE_IMAGE) {
		gs->img_header_len = 0;
		gs->img_sniffed = false;
	}
#endif

	/* Save current request info.
	 * For downloads/images, keep the previous page's request
	 * info so the address bar, cache, and history stay correct
	 * after the download completes. */
#ifdef GEOMYS_DOWNLOAD
	if (new_page_type != PAGE_DOWNLOAD &&
	    new_page_type != PAGE_IMAGE) {
#endif
		strncpy(gs->cur_host, host,
		    sizeof(gs->cur_host) - 1);
		gs->cur_host[sizeof(gs->cur_host) - 1] = '\0';
		gs->cur_port = port;
		gs->cur_type = type;
		strncpy(gs->cur_selector, selector,
		    sizeof(gs->cur_selector) - 1);
		gs->cur_selector[sizeof(gs->cur_selector) - 1] =
		    '\0';
#ifdef GEOMYS_DOWNLOAD
	}
#endif

	/* Send selector */
	if (conn_send_selector(&gs->conn, selector) != noErr) {
		conn_close(&gs->conn);
		return false;
	}
	gs->selector_sent = true;
	gs->receiving = true;
	return true;
}

Boolean
gopher_idle(GopherState *gs)
{
	if (!gs->receiving)
		return false;

	if (gs->conn.state == CONN_STATE_DONE ||
	    gs->conn.state == CONN_STATE_ERROR) {
		gs->receiving = false;
		return true;  /* final update */
	}

	if (gs->conn.state != CONN_STATE_RECEIVING)
		return false;

	conn_idle(&gs->conn);

	if (gs->conn.read_len > 0) {
		gopher_process_data(gs);
		gs->conn.read_len = 0;
		return true;
	}

	/* Check if connection closed after idle */
	if (gs->conn.state == CONN_STATE_DONE ||
	    gs->conn.state == CONN_STATE_ERROR) {
		gs->receiving = false;
		return true;
	}

	return false;
}

/*
 * Process received data incrementally, buffering partial lines.
 */
static void
gopher_process_data(GopherState *gs)
{
	short i;
	char *buf = gs->conn.read_buf;
	short len = gs->conn.read_len;

#ifdef GEOMYS_DOWNLOAD
	if (gs->page_type == PAGE_DOWNLOAD) {
		long count = (long)len;
		OSErr err = FSWrite(gs->dl_refnum, &count, buf);
		if (err != noErr)
			gs->dl_error = true;
		gs->dl_written += count;
		return;
	}

	if (gs->page_type == PAGE_IMAGE) {
		/* First: fill the sniff buffer (26 bytes) */
		if (gs->img_header_len < IMG_HEADER_SIZE) {
			short need = IMG_HEADER_SIZE -
			    gs->img_header_len;
			short copy = (len < need) ? len : need;

			memcpy(gs->img_header + gs->img_header_len,
			    buf, copy);
			gs->img_header_len += copy;
			buf += copy;
			len -= copy;

			if (gs->img_header_len >= IMG_HEADER_SIZE) {
				gs->img_sniffed = true;

				/* Write header to disk immediately
				 * so the saved file is complete */
				if (gs->dl_refnum) {
					long hc = gs->img_header_len;
					OSErr he = FSWrite(
					    gs->dl_refnum,
					    &hc, gs->img_header);
					if (he != noErr)
						gs->dl_error = true;
					gs->dl_written += hc;
				}
			}
		}

		/* Stream remaining data to disk if file is open */
		if (len > 0 && gs->dl_refnum) {
			long count = (long)len;
			OSErr err = FSWrite(gs->dl_refnum,
			    &count, buf);
			if (err != noErr)
				gs->dl_error = true;
			gs->dl_written += count;
		}
		return;
	}
#endif

#ifdef GEOMYS_HTML
	if (gs->page_type == PAGE_HTML) {
		html_process_data(gs, buf, len);
		return;
	}
#endif

	/* CSO phonebook: line-based parsing into text buffer */
	if (gs->page_type == PAGE_TEXT &&
	    gs->cur_type == GOPHER_CSO) {
		cso_process_data(gs, buf, len);
		return;
	}

	if (gs->page_type == PAGE_TEXT) {
		/* Text mode: bulk-copy with memchr/memcpy instead
		 * of byte-at-a-time processing.  Strips \r, converts
		 * \n to \r (Mac line ending), records line starts.
		 * Buffer grows from GOPHER_TEXT_INIT_SIZE to
		 * GOPHER_TEXT_BUFSIZ as needed. */
		char *p = buf;
		char *end = buf + len;
		long avail, cap;

		while (p < end) {
			char *nl = (char *)memchr(p, '\n',
			    end - p);
			char *chunk_end = nl ? nl : end;
			short chunk_len = chunk_end - p;
			short copy_len = chunk_len;

			/* Strip trailing \r (from \r\n endings) */
			if (copy_len > 0 &&
			    p[copy_len - 1] == '\r')
				copy_len--;

			cap = gs->text_buf_capacity;

			/* Grow text_buf if needed */
			if (gs->text_len + copy_len + 2 > cap &&
			    cap < GOPHER_TEXT_BUFSIZ) {
				long new_cap = cap * 2;
				char *new_buf;

				if (new_cap > GOPHER_TEXT_BUFSIZ)
					new_cap = GOPHER_TEXT_BUFSIZ;
				new_buf = NewPtr(new_cap);
				if (new_buf) {
					memcpy(new_buf,
					    gs->text_buf,
					    gs->text_len);
					DisposePtr(gs->text_buf);
					gs->text_buf = new_buf;
					gs->text_buf_capacity =
					    new_cap;
					cap = new_cap;
				}
			}

			/* Copy chunk to text_buf */
			if (copy_len > 0) {
				avail = cap - 1 -
				    gs->text_len;
				if (copy_len > avail)
					copy_len = (short)avail;

				/* Fast path: no interior \r — use
				 * memcpy (common case for normal
				 * Gopher text) */
				if (!memchr(p, '\r', copy_len)) {
					memcpy(gs->text_buf +
					    gs->text_len,
					    p, copy_len);
					gs->text_len += copy_len;
				} else {
					/* Slow path: strip \r */
					short ci;
					for (ci = 0; ci < copy_len;
					    ci++) {
						if (p[ci] != '\r' &&
						    gs->text_len <
						    cap - 1)
							gs->text_buf[
							    gs->text_len++
							    ] = p[ci];
					}
				}
			}

			if (nl) {
				/* Emit Mac line ending */
				if (gs->text_len < cap - 1) {
					gs->text_buf[
					    gs->text_len++] = '\r';

					/* Grow text_lines if needed */
					if (gs->text_lines &&
					    gs->text_line_count >=
					    gs->text_lines_capacity &&
					    gs->text_lines_capacity <
					    GOPHER_MAX_TEXT_LINES) {
						short new_lc =
						    gs->text_lines_capacity
						    * 2;
						long *new_tl;

						if (new_lc >
						    GOPHER_MAX_TEXT_LINES)
							new_lc =
							    GOPHER_MAX_TEXT_LINES;
						new_tl = (long *)NewPtr(
						    (long)new_lc *
						    sizeof(long));
						if (new_tl) {
							memcpy(new_tl,
							    gs->text_lines,
							    (long)gs->
							    text_line_count
							    * sizeof(long));
							DisposePtr((Ptr)
							    gs->text_lines);
							gs->text_lines =
							    new_tl;
							gs->text_lines_capacity
							    = new_lc;
						}
					}

					if (gs->text_lines &&
					    gs->text_line_count <
					    gs->text_lines_capacity) {
						gs->text_lines[
						    gs->text_line_count
						    ] = gs->text_len;
						gs->text_line_count++;
					}
				}
				p = nl + 1;
			} else {
				break;
			}
		}
		gs->text_buf[gs->text_len] = '\0';
		return;
	}

	/* Directory mode: parse line by line */
	for (i = 0; i < len; i++) {
		char c = buf[i];

		if (c == '\n') {
			/* End of line — parse it */
			/* Strip trailing CR */
			if (gs->line_len > 0 &&
			    gs->line_buf[gs->line_len - 1] == '\r')
				gs->line_len--;
			gs->line_buf[gs->line_len] = '\0';

			/* Check for terminating "." */
			if (gs->line_len == 1 &&
			    gs->line_buf[0] == '.') {
				gs->receiving = false;
				return;
			}

			if (gs->line_len > 0)
				gopher_parse_line(gs, gs->line_buf,
				    gs->line_len);

			gs->line_len = 0;
		} else {
			if (gs->line_len < (short)sizeof(gs->line_buf) - 1)
				gs->line_buf[gs->line_len++] = c;
		}
	}
}

/*
 * cso_append_text - Append a string to the text buffer with Mac
 * line endings.  Grows the buffer if needed.  Shared helper for
 * CSO response formatting.
 */
static void
cso_append_text(GopherState *gs, const char *str, short slen)
{
	long cap = gs->text_buf_capacity;

	/* Grow text_buf if needed */
	if (gs->text_len + slen + 2 > cap &&
	    cap < GOPHER_TEXT_BUFSIZ) {
		long new_cap = cap * 2;
		char *new_buf;

		if (new_cap > GOPHER_TEXT_BUFSIZ)
			new_cap = GOPHER_TEXT_BUFSIZ;
		new_buf = NewPtr(new_cap);
		if (new_buf) {
			memcpy(new_buf, gs->text_buf,
			    gs->text_len);
			DisposePtr(gs->text_buf);
			gs->text_buf = new_buf;
			gs->text_buf_capacity = new_cap;
			cap = new_cap;
		}
	}

	if (gs->text_len + slen >= cap)
		slen = (short)(cap - 1 - gs->text_len);
	if (slen <= 0)
		return;

	memcpy(gs->text_buf + gs->text_len, str, slen);
	gs->text_len += slen;
	gs->text_buf[gs->text_len] = '\0';
}

/*
 * cso_end_line - Emit a Mac line ending (\r) and record line
 * start in text_lines index.
 */
static void
cso_end_line(GopherState *gs)
{
	long cap = gs->text_buf_capacity;

	if (gs->text_len >= cap - 1)
		return;

	gs->text_buf[gs->text_len++] = '\r';

	/* Record new line start — grow index if needed */
	if (!gs->text_lines)
		return;

	if (gs->text_line_count >= gs->text_lines_capacity &&
	    gs->text_lines_capacity < GOPHER_MAX_TEXT_LINES) {
		short new_lc = gs->text_lines_capacity * 2;
		long *new_tl;

		if (new_lc > GOPHER_MAX_TEXT_LINES)
			new_lc = GOPHER_MAX_TEXT_LINES;
		new_tl = (long *)NewPtr(
		    (long)new_lc * sizeof(long));
		if (new_tl) {
			memcpy(new_tl, gs->text_lines,
			    (long)gs->text_line_count
			    * sizeof(long));
			DisposePtr((Ptr)gs->text_lines);
			gs->text_lines = new_tl;
			gs->text_lines_capacity = new_lc;
		}
	}

	if (gs->text_line_count < gs->text_lines_capacity) {
		gs->text_lines[gs->text_line_count] =
		    gs->text_len;
		gs->text_line_count++;
	}
}

/*
 * cso_parse_line - Parse a single CSO response line.
 * Format: -200:entry:field: value  (data)
 *         200:Ok.                  (success)
 *         5xx:error message        (error)
 *
 * Extracts "field: value" from data lines, inserts blank
 * lines between entries, shows errors inline.
 */
static void
cso_parse_line(GopherState *gs, const char *line, short len)
{
	const char *p = line;
	const char *end = line + len;
	short code;
	short entry;
	const char *field_start;

	if (len == 0)
		return;

	/* Parse numeric status code (3 digits max) */
	code = 0;
	if (*p == '-') p++;  /* negative = more data */
	while (p < end && *p >= '0' && *p <= '9' &&
	    code < 1000) {
		code = code * 10 + (*p - '0');
		p++;
	}

	/* Success completion — ignore */
	if (code >= 200 && code < 300 && line[0] != '-')
		return;

	/* Error codes (4xx, 5xx) — display inline */
	if (code >= 400) {
		if (*p == ':') p++;
		cso_append_text(gs, "Error: ",  7);
		if (p < end)
			cso_append_text(gs, p, end - p);
		cso_end_line(gs);
		return;
	}

	/* Data line: -200:entry:field: value */
	if (code < 200 || line[0] != '-')
		return;  /* unknown format — skip */

	if (p >= end || *p != ':')
		return;
	p++;  /* skip ':' after code */

	/* Parse entry number (bounded to prevent overflow) */
	entry = 0;
	while (p < end && *p >= '0' && *p <= '9' &&
	    entry < 10000) {
		entry = entry * 10 + (*p - '0');
		p++;
	}

	/* Insert blank line between different entries */
	if (gs->cso_last_entry >= 0 &&
	    entry != gs->cso_last_entry) {
		cso_end_line(gs);
	}
	gs->cso_last_entry = entry;

	if (p >= end || *p != ':')
		return;
	p++;  /* skip ':' after entry number */

	/* Rest is "field: value" — emit as-is, trimming
	 * leading whitespace from field name */
	field_start = p;
	while (field_start < end && *field_start == ' ')
		field_start++;

	if (field_start < end)
		cso_append_text(gs, field_start,
		    end - field_start);
	cso_end_line(gs);
}

/*
 * cso_process_data - Line-buffered CSO response processor.
 * Accumulates data in gs->line_buf, parses complete lines.
 */
static void
cso_process_data(GopherState *gs, char *buf, short len)
{
	short i;

	/* Reset entry tracker on fresh page */
	if (gs->text_len == 0)
		gs->cso_last_entry = -1;

	for (i = 0; i < len; i++) {
		char c = buf[i];

		if (c == '\n') {
			/* Strip trailing CR */
			if (gs->line_len > 0 &&
			    gs->line_buf[gs->line_len - 1] == '\r')
				gs->line_len--;
			gs->line_buf[gs->line_len] = '\0';

			if (gs->line_len > 0)
				cso_parse_line(gs,
				    gs->line_buf,
				    gs->line_len);

			gs->line_len = 0;
		} else {
			if (gs->line_len <
			    (short)sizeof(gs->line_buf) - 1)
				gs->line_buf[gs->line_len++] = c;
		}
	}

	gs->text_buf[gs->text_len] = '\0';
}

/*
 * Parse a single Gopher directory line:
 * type_char display \t selector \t host \t port
 */
static void
gopher_parse_line(GopherState *gs, const char *line, short len)
{
	GopherItem *item;
	const char *p, *end;
	const char *fields[5];
	short field_lens[5];
	short field = 0;
	short copy_len;

	if (gs->item_count >= GOPHER_MAX_ITEMS)
		return;

	/* Grow items array if full */
	if (gs->item_count >= gs->item_capacity) {
		short new_cap;
		long new_size;
		GopherItem *new_items;

		new_cap = gs->item_capacity * 2;
		if (new_cap > GOPHER_MAX_ITEMS)
			new_cap = GOPHER_MAX_ITEMS;
		new_size = (long)sizeof(GopherItem) * new_cap;

		new_items = (GopherItem *)NewPtr(new_size);
		if (!new_items)
			return;  /* out of memory — stop adding */
		memcpy(new_items, gs->items,
		    (long)sizeof(GopherItem) * gs->item_count);
		DisposePtr((Ptr)gs->items);
		gs->items = new_items;
		gs->item_capacity = new_cap;
	}

	item = &gs->items[gs->item_count];
	memset(item, 0, sizeof(GopherItem));

	/* First character is the type */
	item->type = line[0];

	/* Split remaining into tab-separated fields:
	 * display \t selector \t host \t port [\t plus] */
	p = line + 1;
	end = line + len;

	fields[0] = p;
	field_lens[0] = 0;
	field = 0;

	while (p < end && field < 4) {
		if (*p == '\t') {
			field_lens[field] = p - fields[field];
			field++;
			fields[field] = p + 1;
			field_lens[field] = 0;
		}
		p++;
	}
	/* Last field goes to end of line */
	if (field < 5)
		field_lens[field] = end - fields[field];

	/* Copy display text */
	copy_len = field_lens[0];
	if (copy_len > (short)sizeof(item->display) - 1)
		copy_len = sizeof(item->display) - 1;
	memcpy(item->display, fields[0], copy_len);
	item->display[copy_len] = '\0';

	/* Copy selector */
	if (field >= 1) {
		copy_len = field_lens[1];
		if (copy_len > (short)sizeof(item->selector) - 1)
			copy_len = sizeof(item->selector) - 1;
		memcpy(item->selector, fields[1], copy_len);
		item->selector[copy_len] = '\0';
	}

	/* Copy host */
	if (field >= 2) {
		copy_len = field_lens[2];
		if (copy_len > (short)sizeof(item->host) - 1)
			copy_len = sizeof(item->host) - 1;
		memcpy(item->host, fields[2], copy_len);
		item->host[copy_len] = '\0';
	}

	/* Parse port */
	if (field >= 3) {
		item->port = (short)atoi(fields[3]);
	}
	if (item->port == 0)
		item->port = GOPHER_DEFAULT_PORT;

#ifdef GEOMYS_GOPHER_PLUS
	/* Detect Gopher+ support: 5th tab field starting with '+' */
	item->has_plus = 0;
	if (field >= 4) {
		/* There's a 5th field — check for '+' */
		if (field_lens[4] > 0 && fields[4][0] == '+')
			item->has_plus = 1;
	}
#endif

	gs->item_count++;
}

/*
 * Parse a gopher:// URI per RFC 4266.
 * Format: gopher://host[:port][/type[selector]]
 */
Boolean
gopher_parse_uri(const char *uri, char *host, short host_size,
    short *port, char *type, char *selector, short sel_size)
{
	const char *p, *host_start, *host_end;
	short len;

	/* Skip scheme */
	if (strncmp(uri, "gopher://", 9) == 0)
		p = uri + 9;
	else
		p = uri;  /* bare hostname */

	/* Parse host */
	host_start = p;
	host_end = 0L;

	while (*p && *p != ':' && *p != '/')
		p++;

	host_end = p;
	len = host_end - host_start;
	if (len <= 0 || len >= host_size)
		return false;
	memcpy(host, host_start, len);
	host[len] = '\0';

	/* Parse optional port */
	*port = GOPHER_DEFAULT_PORT;
	if (*p == ':') {
		p++;
		*port = (short)atoi(p);
		if (*port <= 0)
			*port = GOPHER_DEFAULT_PORT;
		while (*p && *p != '/')
			p++;
	}

	/* Parse type and selector */
	*type = GOPHER_DIRECTORY;
	selector[0] = '\0';

	if (*p == '/') {
		p++;
		if (*p) {
			*type = *p;
			p++;
			/* Rest is selector */
			len = strlen(p);
			if (len >= sel_size)
				len = sel_size - 1;
			memcpy(selector, p, len);
			selector[len] = '\0';
		}
	}

	return true;
}

void
gopher_build_uri(char *uri, short uri_size, const char *host,
    short port, char type, const char *selector)
{
	if (port != GOPHER_DEFAULT_PORT)
		snprintf(uri, uri_size, "gopher://%s:%d/%c%s",
		    host, port, type, selector);
	else if (selector[0] || type != GOPHER_DIRECTORY)
		snprintf(uri, uri_size, "gopher://%s/%c%s",
		    host, type, selector);
	else
		snprintf(uri, uri_size, "gopher://%s",
		    host);
}
