/*
 * gopherplus.c - Gopher+ protocol support
 *
 * Implements Gopher+ extensions:
 * - Attribute requests (selector\t!)
 * - +INFO, +ADMIN block parsing
 * - +VIEWS content negotiation
 * - Get Info dialog for Gopher+ items
 *
 * Reference: RFC draft-anklesaria-gopher+-02
 */

#ifdef GEOMYS_GOPHER_PLUS

#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Events.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <string.h>
#include <stdio.h>

#include "gopherplus.h"
#include "connection.h"
#include "main.h"
#include "browser.h"
#include "content.h"
#include "dialogs.h"
#include "macutil.h"
#include "settings.h"
#include "tcp.h"

extern GeomysPrefs g_prefs;

/* Response buffer size for attribute fetch */
#define GPLUS_RESP_BUFSIZ  4096L

/* Parse a +ADMIN block to extract Admin and Mod-Date fields.
 * Input: block text after "+ADMIN:\r\n"
 * Returns number of fields parsed. */
short
gopherplus_parse_admin(const char *block, short block_len,
    GopherPlusAdmin *admin)
{
	const char *p, *end, *line_start;
	short fields = 0;

	memset(admin, 0, sizeof(GopherPlusAdmin));

	p = block;
	end = block + block_len;

	while (p < end) {
		/* Skip leading whitespace */
		while (p < end && (*p == ' ' || *p == '\t'))
			p++;

		line_start = p;

		/* Find end of line */
		while (p < end && *p != '\r' && *p != '\n')
			p++;

		{
			short line_len = p - line_start;

			/* Parse "Admin: value" */
			if (line_len > 7 &&
			    strncmp(line_start, "Admin:", 6) == 0) {
				const char *val = line_start + 6;
				short val_len;

				while (*val == ' ') val++;
				val_len = line_len - (val - line_start);
				if (val_len > (short)sizeof(admin->admin) - 1)
					val_len = sizeof(admin->admin) - 1;
				memcpy(admin->admin, val, val_len);
				admin->admin[val_len] = '\0';
				fields++;
			}

			/* Parse "Mod-Date: value" */
			if (line_len > 10 &&
			    strncmp(line_start, "Mod-Date:", 9) == 0) {
				const char *val = line_start + 9;
				short val_len;

				while (*val == ' ') val++;
				val_len = line_len - (val - line_start);
				if (val_len > (short)sizeof(admin->mod_date) - 1)
					val_len = sizeof(admin->mod_date) - 1;
				memcpy(admin->mod_date, val, val_len);
				admin->mod_date[val_len] = '\0';
				fields++;
			}
		}

		/* Skip line ending */
		if (p < end && *p == '\r') p++;
		if (p < end && *p == '\n') p++;
	}

	return fields;
}

/* Parse a +VIEWS block to extract available content types.
 * Input: block text after "+VIEWS:\r\n"
 * Returns number of views parsed. */
short
gopherplus_parse_views(const char *block, short block_len,
    GopherPlusView *views, short max_views)
{
	const char *p, *end, *line_start;
	short count = 0;

	p = block;
	end = block + block_len;

	while (p < end && count < max_views) {
		/* Skip whitespace */
		while (p < end && (*p == ' ' || *p == '\t'))
			p++;

		line_start = p;

		while (p < end && *p != '\r' && *p != '\n')
			p++;

		{
			short line_len = p - line_start;

			if (line_len > 0) {
				/* Format: "content-type language: <size>" */
				const char *colon;
				short type_len;

				colon = memchr(line_start, ':',
				    line_len);
				if (colon) {
					type_len = colon - line_start;
				} else {
					type_len = line_len;
				}

				if (type_len > (short)sizeof(views[count].content_type) - 1)
					type_len = sizeof(views[count].content_type) - 1;
				memcpy(views[count].content_type,
				    line_start, type_len);
				views[count].content_type[type_len] = '\0';

				/* Parse size after colon if present */
				views[count].size = 0;
				if (colon) {
					const char *s = colon + 1;
					while (*s == ' ') s++;
					/* Skip '<' */
					if (*s == '<') s++;
					views[count].size = atol(s);
				}

				count++;
			}
		}

		if (p < end && *p == '\r') p++;
		if (p < end && *p == '\n') p++;
	}

	return count;
}

/* Skip to next line in buffer */
static const char *
skip_line(const char *p, const char *end)
{
	while (p < end && *p != '\r' && *p != '\n')
		p++;
	if (p < end && *p == '\r') p++;
	if (p < end && *p == '\n') p++;
	return p;
}

/* Parse a complete Gopher+ attribute response.
 * Scans for +ADMIN: and +VIEWS: blocks and extracts their content. */
void
gopherplus_parse_response(const char *data, long data_len,
    GopherPlusInfo *info)
{
	const char *p, *end;

	memset(info, 0, sizeof(GopherPlusInfo));

	p = data;
	end = data + data_len;

	/* Skip initial status line (+-1 or +-2) */
	p = skip_line(p, end);

	while (p < end) {
		if (*p == '+') {
			if (end - p >= 7 &&
			    strncmp(p, "+ADMIN:", 7) == 0) {
				const char *block_start;

				p = skip_line(p, end);
				block_start = p;

				/* Block content: indented lines until
				 * next '+' at line start or '.' */
				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);

				info->has_admin = true;
				gopherplus_parse_admin(block_start,
				    p - block_start, &info->admin);

			} else if (end - p >= 7 &&
			    strncmp(p, "+VIEWS:", 7) == 0) {
				const char *block_start;

				p = skip_line(p, end);
				block_start = p;

				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);

				info->has_views = true;
				info->view_count =
				    gopherplus_parse_views(
				    block_start,
				    p - block_start,
				    info->views,
				    GPLUS_MAX_VIEWS);

			} else {
				/* Unknown block — skip header line */
				p = skip_line(p, end);
			}

		} else if (*p == '.') {
			break;  /* End of response */
		} else {
			p = skip_line(p, end);
		}
	}
}

/* Fetch Gopher+ attributes for an item using a temporary
 * synchronous connection. Returns true on success. */
Boolean
gopherplus_fetch_info(const char *host, short port,
    const char *selector, GopherPlusInfo *info)
{
	Connection *conn;
	char sel_buf[300];
	char *resp_buf;
	long resp_len;
	short r;
	unsigned long timeout;

	memset(info, 0, sizeof(GopherPlusInfo));

	/* Heap-allocate Connection (>4KB with read buffer) */
	conn = (Connection *)NewPtrClear(sizeof(Connection));
	if (!conn)
		return false;

	/* Set DNS server from prefs */
	if (g_prefs.dns_server[0]) {
		unsigned long dns_ip;

		dns_ip = ip2long(g_prefs.dns_server);
		if (dns_ip)
			conn->dns_server = dns_ip;
		else
			conn->dns_server = 0x01010101UL;
	} else {
		conn->dns_server = 0x01010101UL;
	}

	/* Connect (DNS resolves synchronously, TCP open is async) */
	if (!conn_connect(conn, host, port, 0L)) {
		DisposePtr((Ptr)conn);
		return false;
	}

	/* Poll until async TCP open completes */
	timeout = TickCount() + 600;  /* 10 second timeout */
	while (conn->state == CONN_STATE_OPENING) {
		EventRecord dummy;

		r = conn_connect_poll(conn);
		if (r < 0) {
			conn_close(conn);
			DisposePtr((Ptr)conn);
			return false;
		}
		if (r == 0)
			break;
		if (TickCount() > timeout) {
			conn_close(conn);
			DisposePtr((Ptr)conn);
			return false;
		}
		WaitNextEvent(0, &dummy, 1, 0L);
	}

	/* Build and send Gopher+ attribute request: selector\t!\r\n */
	snprintf(sel_buf, sizeof(sel_buf), "%s\t!", selector);
	if (conn_send_selector(conn, sel_buf) != noErr) {
		conn_close(conn);
		DisposePtr((Ptr)conn);
		return false;
	}

	/* Read response into heap buffer */
	resp_buf = NewPtr(GPLUS_RESP_BUFSIZ);
	if (!resp_buf) {
		conn_close(conn);
		DisposePtr((Ptr)conn);
		return false;
	}
	resp_len = 0;

	timeout = TickCount() + 600;  /* 10 second read timeout */
	while (conn->state == CONN_STATE_RECEIVING) {
		EventRecord dummy;

		conn_idle(conn);

		if (conn->read_len > 0) {
			long avail = GPLUS_RESP_BUFSIZ - resp_len;
			short copy_len = conn->read_len;

			if (copy_len > avail)
				copy_len = (short)avail;
			if (copy_len > 0) {
				memcpy(resp_buf + resp_len,
				    conn->read_buf, copy_len);
				resp_len += copy_len;
			}
			conn->read_len = 0;
			timeout = TickCount() + 600;
		}

		if (conn->state != CONN_STATE_RECEIVING)
			break;
		if (TickCount() > timeout)
			break;

		WaitNextEvent(0, &dummy, 1, 0L);
	}

	conn_close(conn);
	DisposePtr((Ptr)conn);

	if (resp_len == 0) {
		DisposePtr(resp_buf);
		return false;
	}

	/* Parse the response */
	gopherplus_parse_response(resp_buf, resp_len, info);
	DisposePtr(resp_buf);
	return true;
}

/* Human-readable Gopher type name */
static const char *
gopher_type_label(char type)
{
	switch (type) {
	case '0': return "Text File";
	case '1': return "Directory";
	case '2': return "CSO Phone Book";
	case '3': return "Error";
	case '4': return "BinHex File";
	case '5': return "DOS Archive";
	case '6': return "UUEncoded File";
	case '7': return "Search";
	case '8': return "Telnet Session";
	case '9': return "Binary File";
	case 'g': return "GIF Image";
	case 'I': return "Image";
	case 'T': return "TN3270 Session";
	case 'd': return "Document";
	case 'h': return "HTML Page";
	case 'i': return "Info";
	case 'p': return "PNG Image";
	case 'r': return "RTF Document";
	case 's': return "Sound File";
	default:  return "Unknown";
	}
}

/* Show Get Info dialog for the currently selected Gopher item.
 * Fetches Gopher+ attributes and displays them. */
void
do_getinfo_dialog(void)
{
	short sel_row;
	GopherItem *item;
	GopherPlusInfo info;
	DialogPtr dlg;
	short hit;
	char buf[256];
	Boolean fetched;
	WindowPtr status_win;

	sel_row = content_get_selected_row();
	if (sel_row < 0 ||
	    g_gopher.page_type != PAGE_DIRECTORY)
		return;
	if (sel_row >= g_gopher.item_count)
		return;

	item = &g_gopher.items[sel_row];
	if (item->type == GOPHER_INFO)
		return;

	/* Deactivate address bar so dialog gets keystrokes */
	browser_activate(false);

	/* Show status and fetch Gopher+ attributes */
	status_win = conn_status_show("Getting item info\311");
	fetched = gopherplus_fetch_info(item->host, item->port,
	    item->selector, &info);
	conn_status_close(status_win);

	/* Create and display dialog */
	dlg = GetNewDialog(DLOG_GETINFO_ID, 0L, 0L);
	if (!dlg) {
		browser_activate(true);
		return;
	}

	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	/* Item 2: display name */
	dlg_set_text(dlg, 2, item->display);

	/* Item 3: type */
	snprintf(buf, sizeof(buf), "Type:  %s",
	    gopher_type_label(item->type));
	dlg_set_text(dlg, 3, buf);

	/* Item 4: server */
	snprintf(buf, sizeof(buf), "Server:  %s:%d",
	    item->host, item->port);
	dlg_set_text(dlg, 4, buf);

	/* Item 5: selector */
	if (item->selector[0])
		snprintf(buf, sizeof(buf), "Selector:  %.200s",
		    item->selector);
	else
		snprintf(buf, sizeof(buf), "Selector:  (root)");
	dlg_set_text(dlg, 5, buf);

	/* Item 6: admin (from Gopher+ +ADMIN) */
	if (fetched && info.has_admin && info.admin.admin[0])
		snprintf(buf, sizeof(buf), "Admin:  %.70s",
		    info.admin.admin);
	else
		snprintf(buf, sizeof(buf),
		    "Admin:  (not available)");
	dlg_set_text(dlg, 6, buf);

	/* Item 7: modification date */
	if (fetched && info.has_admin && info.admin.mod_date[0])
		snprintf(buf, sizeof(buf), "Modified:  %.24s",
		    info.admin.mod_date);
	else
		snprintf(buf, sizeof(buf),
		    "Modified:  (not available)");
	dlg_set_text(dlg, 7, buf);

	/* Item 8: available views */
	if (fetched && info.has_views && info.view_count > 0) {
		short i, pos;

		pos = snprintf(buf, sizeof(buf), "Views:  ");
		for (i = 0; i < info.view_count && pos < 240; i++) {
			if (i > 0)
				pos += snprintf(buf + pos,
				    sizeof(buf) - pos, ", ");
			pos += snprintf(buf + pos,
			    sizeof(buf) - pos, "%s",
			    info.views[i].content_type);
		}
		dlg_set_text(dlg, 8, buf);
	} else {
		dlg_set_text(dlg, 8,
		    "Views:  (not available)");
	}

	/* Default button outline */
	setup_default_button_outline(dlg, 9);

	ModalDialog((ModalFilterUPP)std_dlg_filter, &hit);
	DisposeDialog(dlg);

	browser_activate(true);

	/* Invalidate content window behind dialog for redraw */
	if (g_window) {
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		InvalRect(&g_window->portRect);
		SetPort(save);
	}
}

void
gopherplus_init(void)
{
	/* No global state to initialize */
}

#endif /* GEOMYS_GOPHER_PLUS */
