/*
 * html.c - Single-pass streaming HTML tag stripper
 *
 * Converts HTML content to plain text for display in the
 * Gopher browser's text rendering pipeline.  Runs on 68000
 * with minimal memory: no DOM, no backtracking, no malloc.
 */

#ifdef GEOMYS_HTML

#include <string.h>
#include "html.h"
#include "gopher.h"

/* Parser states */
#define HTML_TEXT       0   /* normal text output */
#define HTML_TAG_OPEN  1   /* inside <...> */
#define HTML_ENTITY    2   /* inside &...; */
#define HTML_SKIP      3   /* inside <script>/<style>, skip all */

/* Common HTML entities */
typedef struct {
	const char *name;
	char        ch;
} HtmlEntity;

static const HtmlEntity entity_table[] = {
	{ "amp",  '&'  },
	{ "lt",   '<'  },
	{ "gt",   '>'  },
	{ "quot", '"'  },
	{ "apos", '\'' },
	{ "nbsp", ' '  },
	{ 0L, 0 }
};

/*
 * Case-insensitive string comparison for short tag names.
 */
static Boolean
tag_eq(const char *tag, short tag_len, const char *name)
{
	short i;
	char a, b;

	for (i = 0; name[i]; i++) {
		if (i >= tag_len)
			return false;
		a = tag[i];
		b = name[i];
		/* Lowercase ASCII */
		if (a >= 'A' && a <= 'Z')
			a += 32;
		if (b >= 'A' && b <= 'Z')
			b += 32;
		if (a != b)
			return false;
	}
	return (i == tag_len);
}

/*
 * Emit a single character to text_buf, respecting buffer limits.
 */
static void
html_emit(GopherState *gs, char c)
{
	if (gs->text_len >= GOPHER_TEXT_BUFSIZ - 1)
		return;

	gs->text_buf[gs->text_len++] = c;
	gs->text_buf[gs->text_len] = '\0';
}

/*
 * Emit a carriage return and record a new line in text_lines[].
 */
static void
html_emit_newline(GopherState *gs)
{
	html_emit(gs, '\r');

	if (gs->text_lines &&
	    gs->text_line_count < GOPHER_MAX_TEXT_LINES) {
		gs->text_lines[gs->text_line_count] =
		    gs->text_len;
		gs->text_line_count++;
	}
}

/*
 * Emit a string literal to text_buf.
 */
static void
html_emit_str(GopherState *gs, const char *s)
{
	while (*s)
		html_emit(gs, *s++);
}

/*
 * Resolve an HTML entity name or numeric reference to a character.
 * Returns the character, or '?' if unrecognized.
 */
static char
html_resolve_entity(const char *ent, short len)
{
	const HtmlEntity *p;
	short i;
	char lower[8];

	if (len <= 0 || len > 7)
		return '?';

	/* Numeric entity: &#NN; or &#xNN; */
	if (ent[0] == '#') {
		short val = 0;

		if (len > 1 && (ent[1] == 'x' || ent[1] == 'X')) {
			/* Hex */
			for (i = 2; i < len; i++) {
				char c = ent[i];
				if (c >= '0' && c <= '9')
					val = val * 16 + (c - '0');
				else if (c >= 'a' && c <= 'f')
					val = val * 16 + (c - 'a' + 10);
				else if (c >= 'A' && c <= 'F')
					val = val * 16 + (c - 'A' + 10);
				else
					break;
			}
		} else {
			/* Decimal */
			for (i = 1; i < len; i++) {
				char c = ent[i];
				if (c >= '0' && c <= '9')
					val = val * 10 + (c - '0');
				else
					break;
			}
		}

		if (val > 0 && val < 128)
			return (char)val;
		return '?';
	}

	/* Named entity — lowercase for comparison */
	for (i = 0; i < len && i < 7; i++) {
		lower[i] = ent[i];
		if (lower[i] >= 'A' && lower[i] <= 'Z')
			lower[i] += 32;
	}
	lower[i] = '\0';

	for (p = entity_table; p->name; p++) {
		if (strcmp(lower, p->name) == 0)
			return p->ch;
	}

	return '?';
}

void
html_init(GopherState *gs)
{
	gs->html_state = HTML_TEXT;
	gs->html_tag_len = 0;
	gs->html_ent_len = 0;
	gs->html_in_pre = false;
	gs->html_had_space = false;
	gs->html_in_skip_tag[0] = '\0';
}

void
html_process_data(GopherState *gs, const char *buf, long len)
{
	long i;

	for (i = 0; i < len; i++) {
		char c = buf[i];

		switch (gs->html_state) {

		case HTML_TEXT:
			if (c == '<') {
				gs->html_state = HTML_TAG_OPEN;
				gs->html_tag_len = 0;
			} else if (c == '&') {
				gs->html_state = HTML_ENTITY;
				gs->html_ent_len = 0;
			} else if (gs->html_in_pre) {
				/* Inside <pre>: preserve whitespace */
				if (c == '\n') {
					html_emit_newline(gs);
				} else if (c != '\r') {
					html_emit(gs, c);
				}
			} else {
				/* Normal text: collapse whitespace */
				if (c == ' ' || c == '\t' ||
				    c == '\n' || c == '\r') {
					if (!gs->html_had_space &&
					    gs->text_len > 0) {
						html_emit(gs, ' ');
						gs->html_had_space = true;
					}
				} else {
					html_emit(gs, c);
					gs->html_had_space = false;
				}
			}
			break;

		case HTML_TAG_OPEN:
			if (c == '>') {
				/* Tag complete — process it */
				char tag[16];
				short tlen;
				Boolean is_close = false;
				short ti;

				tlen = gs->html_tag_len;
				if (tlen > 15) tlen = 15;
				memcpy(tag, gs->html_tag, tlen);
				tag[tlen] = '\0';

				/* Strip closing slash: <br/> */
				if (tlen > 0 && tag[tlen - 1] == '/') {
					tag[--tlen] = '\0';
				}

				/* Check for closing tag </...> */
				ti = 0;
				if (tlen > 0 && tag[0] == '/') {
					is_close = true;
					ti = 1;
				}

				/* Truncate at first space/attr */
				{
					short si;
					for (si = ti; si < tlen; si++) {
						if (tag[si] == ' ' ||
						    tag[si] == '\t') {
							tlen = si;
							tag[si] = '\0';
							break;
						}
					}
				}

				/* Check for end of skip block */
				if (gs->html_in_skip_tag[0] &&
				    is_close &&
				    tag_eq(tag + ti, tlen - ti,
				    gs->html_in_skip_tag)) {
					gs->html_in_skip_tag[0] = '\0';
					gs->html_state = HTML_TEXT;
					break;
				}

				/* If we're in a skip block, stay there */
				if (gs->html_in_skip_tag[0]) {
					gs->html_state = HTML_SKIP;
					break;
				}

				/* Block-level tags */
				if (tag_eq(tag + ti, tlen - ti, "br")) {
					html_emit_newline(gs);
					gs->html_had_space = true;
				} else if (tag_eq(tag + ti, tlen - ti,
				    "p")) {
					if (gs->text_len > 0) {
						html_emit_newline(gs);
						html_emit_newline(gs);
					}
					gs->html_had_space = true;
				} else if (tag_eq(tag + ti, tlen - ti,
				    "pre")) {
					if (is_close) {
						gs->html_in_pre = false;
						html_emit_newline(gs);
					} else {
						gs->html_in_pre = true;
						html_emit_newline(gs);
					}
					gs->html_had_space = true;
				} else if (tlen - ti >= 2 &&
				    (tag[ti] == 'h' ||
				    tag[ti] == 'H') &&
				    tag[ti + 1] >= '1' &&
				    tag[ti + 1] <= '6' &&
				    tlen - ti == 2) {
					/* Heading tags h1-h6 */
					if (is_close) {
						html_emit_newline(gs);
					} else if (gs->text_len > 0) {
						html_emit_newline(gs);
						html_emit_newline(gs);
					}
					gs->html_had_space = true;
				} else if (tag_eq(tag + ti, tlen - ti,
				    "li")) {
					if (!is_close) {
						html_emit_newline(gs);
						html_emit_str(gs,
						    "  \245 ");
					}
					gs->html_had_space = true;
				} else if (tag_eq(tag + ti, tlen - ti,
				    "hr")) {
					html_emit_newline(gs);
					html_emit_str(gs,
					    "--------------------------------"
					    "--------");
					html_emit_newline(gs);
					gs->html_had_space = true;
				} else if (tag_eq(tag + ti, tlen - ti,
				    "div") ||
				    tag_eq(tag + ti, tlen - ti,
				    "tr") ||
				    tag_eq(tag + ti, tlen - ti,
				    "dt") ||
				    tag_eq(tag + ti, tlen - ti,
				    "dd")) {
					/* Block elements get line break */
					if (gs->text_len > 0) {
						html_emit_newline(gs);
					}
					gs->html_had_space = true;
				} else if (!is_close &&
				    (tag_eq(tag + ti, tlen - ti,
				    "script") ||
				    tag_eq(tag + ti, tlen - ti,
				    "style"))) {
					/* Enter skip mode */
					short sl = tlen - ti;
					short sj;
					if (sl > 7) sl = 7;
					for (sj = 0; sj < sl; sj++) {
						char sc = tag[ti + sj];
						if (sc >= 'A' &&
						    sc <= 'Z')
							sc += 32;
						gs->html_in_skip_tag[sj] =
						    sc;
					}
					gs->html_in_skip_tag[sl] = '\0';
					gs->html_state = HTML_SKIP;
					break;
				}
				/* All other tags: strip silently */

				gs->html_state = HTML_TEXT;
			} else {
				/* Accumulate tag name */
				if (gs->html_tag_len < 15)
					gs->html_tag[gs->html_tag_len++]
					    = c;
			}
			break;

		case HTML_ENTITY:
			if (c == ';') {
				/* Entity complete — resolve it */
				char ch;

				gs->html_entity[gs->html_ent_len] =
				    '\0';
				ch = html_resolve_entity(
				    gs->html_entity,
				    gs->html_ent_len);
				html_emit(gs, ch);
				gs->html_had_space = false;
				gs->html_state = HTML_TEXT;
			} else if (gs->html_ent_len < 7) {
				gs->html_entity[gs->html_ent_len++] =
				    c;
			} else {
				/* Entity too long — dump as text */
				html_emit(gs, '&');
				{
					short ei;
					for (ei = 0;
					    ei < gs->html_ent_len; ei++)
						html_emit(gs,
						    gs->html_entity[ei]);
				}
				html_emit(gs, c);
				gs->html_had_space = false;
				gs->html_state = HTML_TEXT;
			}
			break;

		case HTML_SKIP:
			/* Inside <script> or <style> — look for < to
			 * check for the closing tag */
			if (c == '<') {
				gs->html_state = HTML_TAG_OPEN;
				gs->html_tag_len = 0;
			}
			break;
		}
	}
}

#endif /* GEOMYS_HTML */
