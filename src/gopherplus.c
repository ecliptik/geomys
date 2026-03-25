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

/* Parse a +SCORE block to extract search relevance score.
 * Input: block text after "+SCORE:\r\n"
 * Returns 0-100 on success, -1 on failure. */
short
gopherplus_parse_score(const char *block, short block_len)
{
	const char *p, *end;
	short val = 0;
	Boolean found = false;

	p = block;
	end = block + block_len;

	/* Skip leading whitespace */
	while (p < end && (*p == ' ' || *p == '\t'))
		p++;

	/* Parse digits */
	while (p < end && *p >= '0' && *p <= '9') {
		val = val * 10 + (*p - '0');
		found = true;
		p++;
		if (val > 100)
			return 100;  /* clamp */
	}

	return found ? val : -1;
}

/* Parse a +ABSTRACT block to extract description text.
 * Joins multi-line text with spaces, trims leading whitespace.
 * Returns length of parsed abstract. */
short
gopherplus_parse_abstract(const char *block, short block_len,
    char *abstract, short max_len)
{
	const char *p, *end, *line_start;
	short pos = 0;

	p = block;
	end = block + block_len;

	while (p < end && pos < max_len - 1) {
		/* Skip leading whitespace */
		while (p < end && (*p == ' ' || *p == '\t'))
			p++;

		line_start = p;

		/* Find end of line */
		while (p < end && *p != '\r' && *p != '\n')
			p++;

		{
			short line_len = p - line_start;

			if (line_len > 0) {
				/* Join lines with space */
				if (pos > 0 && pos < max_len - 1)
					abstract[pos++] = ' ';

				if (pos + line_len > max_len - 1)
					line_len = max_len - 1 - pos;
				if (line_len > 0) {
					memcpy(abstract + pos,
					    line_start, line_len);
					pos += line_len;
				}
			}
		}

		/* Skip line ending */
		if (p < end && *p == '\r') p++;
		if (p < end && *p == '\n') p++;
	}

	/* Truncate with ellipsis if we hit the limit */
	if (pos >= max_len - 4 && pos > 3) {
		pos = max_len - 4;
		abstract[pos++] = '.';
		abstract[pos++] = '.';
		abstract[pos++] = '.';
	}

	abstract[pos] = '\0';
	return pos;
}

/* Parse prompt and optional default from an ASK line.
 * Format: " prompt_text\tdefault_value" */
static void
parse_ask_line(const char *start, const char *end,
    GopherPlusAskField *f)
{
	const char *tab;
	short len;

	while (start < end && *start == ' ')
		start++;

	tab = (const char *)memchr(start, '\t',
	    end - start);

	len = (tab ? tab : end) - start;
	if (len > (short)sizeof(f->prompt) - 1)
		len = sizeof(f->prompt) - 1;
	memcpy(f->prompt, start, len);
	f->prompt[len] = '\0';

	if (tab && tab + 1 < end) {
		start = tab + 1;
		len = end - start;
		if (len > (short)sizeof(f->default_val) - 1)
			len = sizeof(f->default_val) - 1;
		memcpy(f->default_val, start, len);
		f->default_val[len] = '\0';
	}
}

/* Parse a Choose line with tab-separated choices.
 * Format: " label\tchoice1\tchoice2\t..." */
static void
parse_choose_line(const char *start, const char *end,
    GopherPlusAskField *f)
{
	const char *p, *next;
	short len;

	while (start < end && *start == ' ')
		start++;

	/* First field = prompt */
	p = (const char *)memchr(start, '\t',
	    end - start);
	len = (p ? p : end) - start;
	if (len > (short)sizeof(f->prompt) - 1)
		len = sizeof(f->prompt) - 1;
	memcpy(f->prompt, start, len);
	f->prompt[len] = '\0';

	/* Remaining fields = choices */
	f->choice_count = 0;
	if (p) {
		p++;
		while (p < end &&
		    f->choice_count < GPLUS_ASK_MAX_CHOICES) {
			next = (const char *)memchr(p, '\t',
			    end - p);
			len = (next ? next : end) - p;
			if (len > (short)sizeof(f->choices[0])
			    - 1)
				len = sizeof(f->choices[0]) - 1;
			memcpy(f->choices[f->choice_count],
			    p, len);
			f->choices[f->choice_count][len] = '\0';
			f->choice_count++;
			p = next ? next + 1 : end;
		}
	}
}

/* Parse a +ASK block into a form structure.
 * Returns field count, 0 on failure. */
short
gopherplus_parse_ask(const char *block, short block_len,
    GopherPlusAskForm *form)
{
	const char *p = block;
	const char *end = block + block_len;

	form->field_count = 0;

	while (p < end &&
	    form->field_count < GPLUS_ASK_MAX_FIELDS) {
		const char *line_start, *line_end;
		short line_len;
		GopherPlusAskField *f;

		while (p < end && (*p == ' ' || *p == '\t'))
			p++;
		line_start = p;
		while (p < end && *p != '\r' && *p != '\n')
			p++;
		line_end = p;
		line_len = line_end - line_start;
		if (p < end && *p == '\r') p++;
		if (p < end && *p == '\n') p++;
		if (line_len == 0) continue;

		f = &form->fields[form->field_count];
		memset(f, 0, sizeof(*f));

		if (line_len > 4 &&
		    strncmp(line_start, "Ask:", 4) == 0) {
			f->type = ASK_TYPE_ASK;
			parse_ask_line(line_start + 4,
			    line_end, f);
			form->field_count++;
		} else if (line_len > 5 &&
		    strncmp(line_start, "AskP:", 5) == 0) {
			f->type = ASK_TYPE_ASKP;
			parse_ask_line(line_start + 5,
			    line_end, f);
			form->field_count++;
		} else if (line_len > 5 &&
		    strncmp(line_start, "AskL:", 5) == 0) {
			f->type = ASK_TYPE_ASKL;
			parse_ask_line(line_start + 5,
			    line_end, f);
			form->field_count++;
		} else if (line_len > 7 &&
		    strncmp(line_start, "Choose:", 7) == 0) {
			f->type = ASK_TYPE_CHOOSE;
			parse_choose_line(line_start + 7,
			    line_end, f);
			form->field_count++;
		} else if (line_len > 7 &&
		    strncmp(line_start, "Select:", 7) == 0) {
			f->type = ASK_TYPE_SELECT;
			parse_ask_line(line_start + 7,
			    line_end, f);
			form->field_count++;
		}
		/* AskF, ChooseF: skip silently */
	}
	return form->field_count;
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

			} else if (end - p >= 10 &&
			    strncmp(p, "+ABSTRACT:", 10) == 0) {
				const char *block_start;

				p = skip_line(p, end);
				block_start = p;

				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);

				info->has_abstract = true;
				gopherplus_parse_abstract(
				    block_start,
				    p - block_start,
				    info->abstract,
				    sizeof(info->abstract));

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

			} else if (end - p >= 7 &&
			    strncmp(p, "+SCORE:", 7) == 0) {
				const char *block_start;

				p = skip_line(p, end);
				block_start = p;

				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);

				info->score =
				    gopherplus_parse_score(
				    block_start,
				    p - block_start);
				if (info->score >= 0)
					info->has_score = true;

			} else if (end - p >= 5 &&
			    strncmp(p, "+ASK:", 5) == 0) {
				const char *block_start;

				p = skip_line(p, end);
				block_start = p;

				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);

				/* Heap-allocate form */
				if (!info->ask_form) {
					info->ask_form =
					    (GopherPlusAskForm *)
					    NewPtrClear(sizeof(
					    GopherPlusAskForm));
				}
				if (info->ask_form) {
					GopherPlusAskForm *af =
					    (GopherPlusAskForm *)
					    info->ask_form;
					gopherplus_parse_ask(
					    block_start,
					    p - block_start,
					    af);
					if (af->field_count > 0)
						info->has_ask = true;
				}

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

/* --- Programmatic DITL builder for +ASK dialogs --- */

/* DITL item type constants */
#define DI_BUTTON   4
#define DI_CHECKBOX 5
#define DI_RADIO    6
#define DI_STATTEXT 8
#define DI_EDITTEXT 16
#define DI_USERITEM 0
#define DI_DISABLED 128

typedef struct {
	Handle  h;
	short   count;
	long    size;
} DITLBuild;

static void
ditl_begin(DITLBuild *d)
{
	d->h = NewHandle(2L);
	d->count = 0;
	d->size = 2;
	if (d->h)
		*(short *)*d->h = -1;
}

static void
ditl_add(DITLBuild *d, short top, short left, short bot,
    short right, short type, const char *text)
{
	short tlen = text ? (short)strlen(text) : 0;
	short raw = 14 + tlen;
	short padded = (raw + 1) & ~1;
	char *p;

	SetHandleSize(d->h, d->size + padded);
	if (MemError() != noErr) return;

	p = *d->h + d->size;
	*(long *)p = 0L; p += 4;
	*(short *)p = top; p += 2;
	*(short *)p = left; p += 2;
	*(short *)p = bot; p += 2;
	*(short *)p = right; p += 2;
	*p++ = (char)type;
	*p++ = (char)tlen;
	if (tlen > 0) {
		memcpy(p, text, tlen);
		p += tlen;
	}
	if (padded > raw) *p = 0;

	d->size += padded;
	d->count++;
	*(short *)(*d->h) = d->count - 1;
}

/* Show +ASK form dialog built programmatically.
 * Returns 0 on Send, -1 on Cancel.
 * On Send, field values are updated in form->fields. */
short
do_ask_dialog(GopherPlusAskForm *form, const char *title)
{
	DITLBuild ditl;
	DialogPtr dlg;
	Rect bounds;
	short y, i, j, hit;
	short width = 340;
	short field_start[GPLUS_ASK_MAX_FIELDS];
	short first_edit = 0;
	short outline_item;
	Str255 ptitle;
	short tlen;

	/* Calculate dialog height */
	y = 13;
	for (i = 0; i < form->field_count; i++) {
		GopherPlusAskField *f = &form->fields[i];

		y += 16 + 4;  /* label + gap */
		switch (f->type) {
		case ASK_TYPE_ASK:
		case ASK_TYPE_ASKP:
			y += 20; break;
		case ASK_TYPE_ASKL:
			y += 48; break;
		case ASK_TYPE_CHOOSE:
			y += f->choice_count * 18; break;
		case ASK_TYPE_SELECT:
			y += 18; break;
		}
		y += 8;
	}
	y += 30 + 13;  /* buttons + bottom margin */
	if (y > 260) y = 260;

	/* Build DITL in memory */
	ditl_begin(&ditl);
	if (!ditl.h)
		return -1;

	/* Item 1: Send (default), Item 2: Cancel */
	ditl_add(&ditl, y - 43, width - 83,
	    y - 23, width - 13, DI_BUTTON, "Send");
	ditl_add(&ditl, y - 43, width - 168,
	    y - 23, width - 98, DI_BUTTON, "Cancel");

	/* Build form fields */
	{
		short cy = 13;

		for (i = 0; i < form->field_count; i++) {
			GopherPlusAskField *f =
			    &form->fields[i];

			if (cy + 36 > y - 43)
				break;  /* out of space */

			field_start[i] = ditl.count + 1;

			/* Label */
			ditl_add(&ditl, cy, 13,
			    cy + 16, width - 13,
			    DI_STATTEXT | DI_DISABLED,
			    f->prompt);
			cy += 16 + 4;

			switch (f->type) {
			case ASK_TYPE_ASK:
			case ASK_TYPE_ASKP:
				ditl_add(&ditl, cy, 13,
				    cy + 20, width - 13,
				    DI_EDITTEXT,
				    f->default_val);
				if (!first_edit)
					first_edit = ditl.count;
				cy += 20;
				break;

			case ASK_TYPE_ASKL:
				ditl_add(&ditl, cy, 13,
				    cy + 48, width - 13,
				    DI_EDITTEXT,
				    f->default_val);
				if (!first_edit)
					first_edit = ditl.count;
				cy += 48;
				break;

			case ASK_TYPE_CHOOSE:
				for (j = 0;
				    j < f->choice_count; j++) {
					ditl_add(&ditl, cy, 20,
					    cy + 16, width - 20,
					    DI_RADIO,
					    f->choices[j]);
					cy += 18;
				}
				break;

			case ASK_TYPE_SELECT:
				ditl_add(&ditl, cy, 20,
				    cy + 16, width - 20,
				    DI_CHECKBOX, f->prompt);
				cy += 18;
				break;
			}
			cy += 8;
		}
	}

	/* Outline UserItem for default button */
	outline_item = ditl.count + 1;
	ditl_add(&ditl, y - 47, width - 87,
	    y - 19, width - 9,
	    DI_USERITEM | DI_DISABLED, "");

	/* Create dialog */
	SetRect(&bounds, 80, 90, 80 + width, 90 + y);
	tlen = strlen(title);
	if (tlen > 63) tlen = 63;
	ptitle[0] = (unsigned char)tlen;
	memcpy(ptitle + 1, title, tlen);

	dlg = NewDialog(0L, &bounds, ptitle, true,
	    movableDBoxProc, (WindowPtr)-1L, false,
	    0L, ditl.h);
	if (!dlg) {
		DisposeHandle(ditl.h);
		return -1;
	}

	center_dialog_on_screen(dlg);
	setup_default_button_outline(dlg, outline_item);

	/* Initialize radio buttons and checkboxes */
	for (i = 0; i < form->field_count; i++) {
		GopherPlusAskField *f = &form->fields[i];

		if (f->type == ASK_TYPE_CHOOSE &&
		    f->choice_count > 0) {
			short itype;
			Handle ih;
			Rect ib;

			GetDialogItem(dlg,
			    field_start[i] + 1,
			    &itype, &ih, &ib);
			SetControlValue(
			    (ControlHandle)ih, 1);
			f->selected = 0;
		}
		if (f->type == ASK_TYPE_SELECT)
			f->selected = 0;
	}

	if (first_edit > 0)
		SelectDialogItemText(dlg, first_edit,
		    0, 32767);

	/* Modal loop */
	do {
		ModalDialog(
		    (ModalFilterUPP)std_dlg_filter, &hit);

		for (i = 0; i < form->field_count; i++) {
			GopherPlusAskField *f =
			    &form->fields[i];
			short base = field_start[i] + 1;

			if (f->type == ASK_TYPE_CHOOSE) {
				for (j = 0;
				    j < f->choice_count;
				    j++) {
					if (hit == base + j &&
					    j != f->selected) {
						short it;
						Handle ih;
						Rect ib;

						GetDialogItem(dlg,
						    base +
						    f->selected,
						    &it, &ih,
						    &ib);
						SetControlValue(
						    (ControlHandle)
						    ih, 0);
						GetDialogItem(dlg,
						    hit,
						    &it, &ih,
						    &ib);
						SetControlValue(
						    (ControlHandle)
						    ih, 1);
						f->selected = j;
					}
				}
			}

			if (f->type == ASK_TYPE_SELECT &&
			    hit == base) {
				short it;
				Handle ih;
				Rect ib;
				short val;

				GetDialogItem(dlg, hit,
				    &it, &ih, &ib);
				val = GetControlValue(
				    (ControlHandle)ih);
				SetControlValue(
				    (ControlHandle)ih,
				    val ? 0 : 1);
				f->selected = val ? 0 : 1;
			}
		}
	} while (hit != 1 && hit != 2);

	/* Extract text field values on Send */
	if (hit == 1) {
		for (i = 0; i < form->field_count; i++) {
			GopherPlusAskField *f =
			    &form->fields[i];

			if (f->type == ASK_TYPE_ASK ||
			    f->type == ASK_TYPE_ASKP ||
			    f->type == ASK_TYPE_ASKL) {
				Str255 pstr;
				short it;
				Handle ih;
				Rect ib;
				short slen;

				GetDialogItem(dlg,
				    field_start[i] + 1,
				    &it, &ih, &ib);
				GetDialogItemText(ih, pstr);
				slen = pstr[0];
				if (slen >
				    (short)sizeof(
				    f->default_val) - 1)
					slen = sizeof(
					    f->default_val) - 1;
				memcpy(f->default_val,
				    pstr + 1, slen);
				f->default_val[slen] = '\0';
			}
		}
	}

	DisposeDialog(dlg);
	return (hit == 1) ? 0 : -1;
}

/* Show view selection dialog with radio buttons for each
 * available view.  Returns chosen index (0-based) or -1. */
short
do_view_select_dialog(GopherPlusInfo *info)
{
	DialogPtr dlg;
	short hit, i, chosen;

	dlg = GetNewDialog(DLOG_VIEW_SELECT_ID, 0L, 0L);
	if (!dlg)
		return -1;

	center_dialog_on_screen(dlg);

	/* Set radio button titles and hide unused slots */
	for (i = 0; i < GPLUS_MAX_VIEWS; i++) {
		short item_num = i + 3;
		short itype;
		Handle ihandle;
		Rect ibox;

		GetDialogItem(dlg, item_num,
		    &itype, &ihandle, &ibox);

		if (i < info->view_count) {
			Str255 pstr;
			const char *ct =
			    info->views[i].content_type;
			short slen = strlen(ct);

			if (slen > 255) slen = 255;
			pstr[0] = (unsigned char)slen;
			memcpy(pstr + 1, ct, slen);
			SetControlTitle(
			    (ControlHandle)ihandle, pstr);
		} else {
			/* Move offscreen to hide */
			SetRect(&ibox, -1000, -1000,
			    -980, -990);
			SetDialogItem(dlg, item_num,
			    itype, ihandle, &ibox);
		}
	}

	/* Select first radio button */
	chosen = 0;
	{
		short itype;
		Handle ihandle;
		Rect ibox;

		GetDialogItem(dlg, 3,
		    &itype, &ihandle, &ibox);
		SetControlValue(
		    (ControlHandle)ihandle, 1);
	}

	setup_default_button_outline(dlg, 11);

	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter,
		    &hit);

		/* Radio button toggle */
		if (hit >= 3 &&
		    hit < 3 + info->view_count) {
			short new_sel = hit - 3;

			if (new_sel != chosen) {
				short itype;
				Handle ihandle;
				Rect ibox;

				/* Deselect old */
				GetDialogItem(dlg,
				    chosen + 3,
				    &itype, &ihandle,
				    &ibox);
				SetControlValue(
				    (ControlHandle)ihandle,
				    0);

				/* Select new */
				GetDialogItem(dlg, hit,
				    &itype, &ihandle,
				    &ibox);
				SetControlValue(
				    (ControlHandle)ihandle,
				    1);
				chosen = new_sel;
			}
		}
	} while (hit != 1 && hit != 2);

	DisposeDialog(dlg);
	return (hit == 1) ? chosen : -1;
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

	/* Try bulk cache first, then individual fetch */
	memset(&info, 0, sizeof(info));
	fetched = false;

	if (g_prefs.gopher_plus) {
		GopherPlusCache *cache =
		    (GopherPlusCache *)g_gopher.gplus_cache;

		/* Lazy bulk fetch on first Get Info for a directory */
		if (!cache) {
			cache = (GopherPlusCache *)NewPtrClear(
			    sizeof(GopherPlusCache));
			if (cache) {
				g_gopher.gplus_cache = cache;
				status_win = conn_status_show(
				    "Loading directory "
				    "attributes\311");
				gopherplus_fetch_bulk(
				    g_gopher.cur_host,
				    g_gopher.cur_port,
				    g_gopher.cur_selector,
				    cache);
				conn_status_close(status_win);
			}
		}

		/* Check cache for this item */
		if (cache && cache->fetched) {
			const GopherPlusCacheEntry *ce;

			ce = gopherplus_cache_lookup(cache,
			    item->selector);
			if (ce) {
				if (ce->has_abstract) {
					info.has_abstract = true;
					strncpy(info.abstract,
					    ce->abstract,
					    sizeof(info.abstract)
					    - 1);
				}
				if (ce->has_score) {
					info.has_score = true;
					info.score = ce->score;
				}
			}
		}
	}

	/* Full individual fetch for complete info */
	if (g_prefs.gopher_plus) {
		status_win = conn_status_show(
		    "Getting item info\311");
		fetched = gopherplus_fetch_info(item->host,
		    item->port, item->selector, &info);
		conn_status_close(status_win);
	}

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

	/* Item 9: abstract (from Gopher+ +ABSTRACT) */
	if (fetched && info.has_abstract && info.abstract[0])
		snprintf(buf, sizeof(buf), "Abstract:  %.200s",
		    info.abstract);
	else
		snprintf(buf, sizeof(buf),
		    "Abstract:  (not available)");
	dlg_set_text(dlg, 9, buf);

	/* Item 10: "Choose View..." — dim when <= 1 view */
	{
		short itype;
		Handle ihandle;
		Rect ibox;

		GetDialogItem(dlg, 10, &itype,
		    &ihandle, &ibox);
		if (!fetched || !info.has_views ||
		    info.view_count <= 1)
			HiliteControl(
			    (ControlHandle)ihandle, 255);
	}

	/* Item 11: "Fill Form..." — dim when no +ASK */
	{
		short itype;
		Handle ihandle;
		Rect ibox;

		GetDialogItem(dlg, 11, &itype,
		    &ihandle, &ibox);
		if (!fetched || !info.has_ask ||
		    !info.ask_form)
			HiliteControl(
			    (ControlHandle)ihandle, 255);
	}

	/* Default button outline */
	setup_default_button_outline(dlg, 12);

	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter,
		    &hit);

		if (hit == 10 && fetched &&
		    info.has_views &&
		    info.view_count > 1) {
			short chosen;

			chosen = do_view_select_dialog(&info);
			if (chosen >= 0) {
				char url[400];

				DisposeDialog(dlg);

				/* Set view and navigate */
				strncpy(
				    g_gopher.gplus_view,
				    info.views[chosen].
				    content_type,
				    sizeof(g_gopher.
				    gplus_view) - 1);
				g_gopher.gplus_view[
				    sizeof(g_gopher.
				    gplus_view) - 1] = '\0';

				gopher_build_uri(url,
				    sizeof(url),
				    item->host,
				    item->port,
				    item->type,
				    item->selector);
				browser_activate(true);
				do_navigate_url(url);
				if (info.ask_form)
					DisposePtr(
					    (Ptr)info.ask_form);
				return;
			}
		}

		if (hit == 11 && fetched &&
		    info.has_ask && info.ask_form) {
			if (do_ask_dialog(
			    (GopherPlusAskForm *)
			    info.ask_form,
			    item->display) == 0) {
				char url[400];

				DisposeDialog(dlg);

				gopher_build_uri(url,
				    sizeof(url),
				    item->host,
				    item->port,
				    item->type,
				    item->selector);
				browser_activate(true);
				do_navigate_url(url);

				/* Transfer form ownership
				 * to gopher state for
				 * ASK submission */
				g_gopher.gplus_ask_form =
				    info.ask_form;
				info.ask_form = 0L;
				return;
			}
		}
	} while (hit != 1);

	DisposeDialog(dlg);
	browser_activate(true);

	/* Free ASK form if not transferred */
	if (info.ask_form) {
		DisposePtr((Ptr)info.ask_form);
		info.ask_form = 0L;
	}

	/* Invalidate content window behind dialog for redraw */
	if (g_window) {
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		InvalRect(&g_window->portRect);
		SetPort(save);
	}
}

/* --- Bulk attribute cache --- */

void
gopherplus_cache_clear(GopherPlusCache *cache)
{
	if (!cache)
		return;
	cache->count = 0;
	cache->fetched = false;
}

const GopherPlusCacheEntry *
gopherplus_cache_lookup(const GopherPlusCache *cache,
    const char *selector)
{
	short i;

	if (!cache || !cache->fetched)
		return 0L;

	for (i = 0; i < cache->count; i++) {
		if (strcmp(cache->entries[i].selector,
		    selector) == 0)
			return &cache->entries[i];
	}
	return 0L;
}

/* Parse a bulk attribute response (selector\t$\r\n).
 * The response contains multiple +INFO blocks, each followed
 * by +ADMIN, +VIEWS, +ABSTRACT, +SCORE blocks for that item.
 * Uses a sequential cursor — O(N) for N blocks. */
static void
gopherplus_parse_bulk(const char *data, long data_len,
    GopherPlusCache *cache)
{
	const char *p, *end;
	GopherPlusCacheEntry *cur = 0L;

	cache->count = 0;

	p = data;
	end = data + data_len;

	/* Skip initial status line */
	p = skip_line(p, end);

	while (p < end) {
		if (*p == '+') {
			if (end - p >= 6 &&
			    strncmp(p, "+INFO:", 6) == 0) {
				/* New item — extract selector from
				 * the +INFO line.  Format:
				 * +INFO: type display \t sel \t host \t port */
				const char *line_start;
				short line_len;
				const char *tab1, *tab2, *lp;

				p = skip_line(p, end);
				line_start = p;

				/* Read the info line (indented) */
				while (p < end &&
				    (*p == ' ' || *p == '\t'))
					p++;
				line_start = p;
				while (p < end &&
				    *p != '\r' && *p != '\n')
					p++;
				line_len = p - line_start;
				if (p < end && *p == '\r') p++;
				if (p < end && *p == '\n') p++;

				/* Extract selector: 2nd tab field */
				tab1 = 0L;
				tab2 = 0L;
				for (lp = line_start;
				    lp < line_start + line_len;
				    lp++) {
					if (*lp == '\t') {
						if (!tab1)
							tab1 = lp;
						else if (!tab2) {
							tab2 = lp;
							break;
						}
					}
				}

				if (tab1 &&
				    cache->count < GPLUS_CACHE_MAX) {
					short sel_len;
					const char *sel_start =
					    tab1 + 1;

					if (tab2)
						sel_len = tab2 -
						    sel_start;
					else
						sel_len =
						    (line_start +
						    line_len) -
						    sel_start;
					if (sel_len > (short)sizeof(
					    cache->entries[0].selector)
					    - 1)
						sel_len = sizeof(
						    cache->entries[0].
						    selector) - 1;

					cur = &cache->entries[
					    cache->count];
					memset(cur, 0,
					    sizeof(*cur));
					cur->score = -1;
					memcpy(cur->selector,
					    sel_start, sel_len);
					cur->selector[sel_len] =
					    '\0';
					cache->count++;
				} else {
					cur = 0L;
				}

			} else if (end - p >= 10 &&
			    strncmp(p, "+ABSTRACT:", 10) == 0) {
				const char *block_start;

				p = skip_line(p, end);
				block_start = p;

				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);

				if (cur) {
					gopherplus_parse_abstract(
					    block_start,
					    p - block_start,
					    cur->abstract,
					    sizeof(cur->abstract));
					if (cur->abstract[0])
						cur->has_abstract =
						    true;
				}

			} else if (end - p >= 7 &&
			    strncmp(p, "+SCORE:", 7) == 0) {
				const char *block_start;

				p = skip_line(p, end);
				block_start = p;

				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);

				if (cur) {
					cur->score =
					    gopherplus_parse_score(
					    block_start,
					    p - block_start);
					if (cur->score >= 0)
						cur->has_score = true;
				}

			} else if (end - p >= 5 &&
			    strncmp(p, "+ASK:", 5) == 0) {
				/* Mark item as having +ASK */
				p = skip_line(p, end);
				while (p < end &&
				    *p != '+' && *p != '.')
					p = skip_line(p, end);
				if (cur)
					cur->has_ask = true;

			} else {
				/* Skip other blocks (+ADMIN, +VIEWS,
				 * etc.) — not cached in bulk */
				p = skip_line(p, end);
			}

		} else if (*p == '.') {
			break;
		} else {
			p = skip_line(p, end);
		}
	}
}

/* Fetch bulk Gopher+ attributes for a directory.
 * Sends selector\t$\r\n, parses multi-item response. */
Boolean
gopherplus_fetch_bulk(const char *host, short port,
    const char *selector, GopherPlusCache *cache)
{
	Connection *conn;
	char sel_buf[300];
	char *resp_buf;
	long resp_len;
	short r;
	unsigned long timeout;

	cache->count = 0;
	cache->fetched = true;  /* mark attempted even on failure */

	/* Heap-allocate Connection */
	conn = (Connection *)NewPtrClear(sizeof(Connection));
	if (!conn)
		return false;

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

	if (!conn_connect(conn, host, port, 0L)) {
		DisposePtr((Ptr)conn);
		return false;
	}

	/* Poll async open */
	timeout = TickCount() + 600;
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

	/* Send bulk attribute request: selector\t$\r\n */
	snprintf(sel_buf, sizeof(sel_buf), "%s\t$", selector);
	if (conn_send_selector(conn, sel_buf) != noErr) {
		conn_close(conn);
		DisposePtr((Ptr)conn);
		return false;
	}

	/* Read response */
	resp_buf = NewPtr(GPLUS_BULK_BUFSIZ);
	if (!resp_buf) {
		conn_close(conn);
		DisposePtr((Ptr)conn);
		return false;
	}
	resp_len = 0;

	timeout = TickCount() + 900;  /* 15 second read timeout */
	while (conn->state == CONN_STATE_RECEIVING) {
		EventRecord dummy;

		conn_idle(conn);

		if (conn->read_len > 0) {
			long avail = GPLUS_BULK_BUFSIZ - resp_len;
			short copy_len = conn->read_len;

			if (copy_len > avail)
				copy_len = (short)avail;
			if (copy_len > 0) {
				memcpy(resp_buf + resp_len,
				    conn->read_buf, copy_len);
				resp_len += copy_len;
			}
			conn->read_len = 0;
			timeout = TickCount() + 900;
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

	gopherplus_parse_bulk(resp_buf, resp_len, cache);
	DisposePtr(resp_buf);
	return (cache->count > 0);
}

void
gopherplus_init(void)
{
	/* No global state to initialize */
}

#endif /* GEOMYS_GOPHER_PLUS */
