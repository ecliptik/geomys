/*
 * gopherplus.c - Gopher+ protocol support
 *
 * Implements Gopher+ extensions:
 * - Attribute requests (selector\t!)
 * - +INFO, +ADMIN block parsing
 * - +VIEWS content negotiation
 * - +ASK interactive forms (Phase 14c)
 *
 * Reference: RFC draft-anklesaria-gopher+-02
 */

#ifdef GEOMYS_GOPHER_PLUS

#include <Memory.h>
#include <string.h>
#include <stdio.h>

#include "gopherplus.h"

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

void
gopherplus_init(void)
{
	/* No global state to initialize */
}

#endif /* GEOMYS_GOPHER_PLUS */
