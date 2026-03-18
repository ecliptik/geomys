/*
 * gopher.c - RFC 1436 Gopher protocol engine
 */

#include <Memory.h>
#include <Quickdraw.h>
#include <Events.h>
#include <Windows.h>
#include <Dialogs.h>
#include <ToolUtils.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gopher.h"
#include "connection.h"
#include "dialogs.h"
#include "macutil.h"
#include "main.h"

/* Forward declarations */
static void gopher_parse_line(GopherState *gs, const char *line, short len);
static void gopher_process_data(GopherState *gs);

void
gopher_init(GopherState *gs)
{
	memset(gs, 0, sizeof(GopherState));
	gs->conn.dns_server = 0x01010101UL;  /* 1.1.1.1 default */
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

	if (gs->text_buf) {
		DisposePtr(gs->text_buf);
		gs->text_buf = 0L;
	}
	gs->text_len = 0;

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
		    (long)sizeof(GopherItem) * GOPHER_MAX_ITEMS);
		if (!new_items)
			return false;
	} else if (type == GOPHER_TEXT) {
		new_page_type = PAGE_TEXT;
		new_text = NewPtr(GOPHER_TEXT_BUFSIZ);
		if (!new_text)
			return false;
	} else {
		new_page_type = PAGE_DIRECTORY;
		new_items = (GopherItem *)NewPtr(
		    (long)sizeof(GopherItem) * GOPHER_MAX_ITEMS);
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
		return false;
	}

	/* Connection succeeded — now clear old page and switch */
	gopher_clear_page(gs);

	gs->page_type = new_page_type;
	gs->items = new_items;
	gs->text_buf = new_text;

	/* Save current request info */
	strncpy(gs->cur_host, host, sizeof(gs->cur_host) - 1);
	gs->cur_host[sizeof(gs->cur_host) - 1] = '\0';
	gs->cur_port = port;
	gs->cur_type = type;
	strncpy(gs->cur_selector, selector,
	    sizeof(gs->cur_selector) - 1);
	gs->cur_selector[sizeof(gs->cur_selector) - 1] = '\0';

	/* Send selector */
	conn_send_selector(&gs->conn, selector);
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

	if (gs->page_type == PAGE_TEXT) {
		/* Text mode: append raw data to text buffer */
		for (i = 0; i < len; i++) {
			if (gs->text_len < GOPHER_TEXT_BUFSIZ - 1) {
				/* Convert LF to CR for Mac line endings */
				if (buf[i] == '\n')
					gs->text_buf[gs->text_len++] = '\r';
				else if (buf[i] != '\r')
					gs->text_buf[gs->text_len++] =
					    buf[i];
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
 * Parse a single Gopher directory line:
 * type_char display \t selector \t host \t port
 */
static void
gopher_parse_line(GopherState *gs, const char *line, short len)
{
	GopherItem *item;
	const char *p, *end;
	const char *fields[4];
	short field_lens[4];
	short field = 0;
	short copy_len;

	if (gs->item_count >= GOPHER_MAX_ITEMS)
		return;

	item = &gs->items[gs->item_count];
	memset(item, 0, sizeof(GopherItem));

	/* First character is the type */
	item->type = line[0];

	/* Split remaining into tab-separated fields:
	 * display \t selector \t host \t port */
	p = line + 1;
	end = line + len;

	fields[0] = p;
	field_lens[0] = 0;
	field = 0;

	while (p < end && field < 3) {
		if (*p == '\t') {
			field_lens[field] = p - fields[field];
			field++;
			fields[field] = p + 1;
			field_lens[field] = 0;
		}
		p++;
	}
	/* Last field goes to end of line */
	if (field < 4)
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
