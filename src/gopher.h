/*
 * gopher.h - RFC 1436 Gopher protocol engine
 */

#ifndef GOPHER_H
#define GOPHER_H

#include "connection.h"

/* Maximum items in a directory listing */
#define GOPHER_MAX_ITEMS    200

/* Text buffer size for type-0 content */
#define GOPHER_TEXT_BUFSIZ  (32L * 1024L)

/* Gopher item types (RFC 1436 canonical) */
#define GOPHER_TEXT         '0'
#define GOPHER_DIRECTORY    '1'
#define GOPHER_CSO          '2'
#define GOPHER_ERROR        '3'
#define GOPHER_BINHEX       '4'
#define GOPHER_DOS          '5'
#define GOPHER_UUENCODE     '6'
#define GOPHER_SEARCH       '7'
#define GOPHER_TELNET       '8'
#define GOPHER_BINARY       '9'
#define GOPHER_GIF          'g'
#define GOPHER_IMAGE        'I'
#define GOPHER_TN3270       'T'

/* Non-canonical types */
#define GOPHER_DOC          'd'
#define GOPHER_HTML         'h'
#define GOPHER_INFO         'i'
#define GOPHER_PNG          'p'
#define GOPHER_RTF          'r'
#define GOPHER_SOUND        's'

/* Page content types */
#define PAGE_NONE           0
#define PAGE_DIRECTORY      1
#define PAGE_TEXT           2
#define PAGE_ERROR          3

typedef struct {
	char    type;
	char    display[80];
	char    selector[256];
	char    host[64];
	short   port;
} GopherItem;

typedef struct {
	/* Current page content */
	short       page_type;      /* PAGE_DIRECTORY, PAGE_TEXT, etc. */

	/* Directory listing */
	GopherItem  *items;         /* NewPtr-allocated array */
	short       item_count;

	/* Text content */
	char        *text_buf;      /* NewPtr-allocated, GOPHER_TEXT_BUFSIZ */
	long        text_len;

	/* Connection */
	Connection  conn;

	/* Parse state for incremental receive */
	char        line_buf[512];
	short       line_len;
	Boolean     selector_sent;
	Boolean     receiving;

	/* Current request */
	char        cur_host[64];
	short       cur_port;
	char        cur_selector[256];
	char        cur_type;
} GopherState;

/* Initialize gopher state — call once at startup */
void gopher_init(GopherState *gs);

/* Clean up gopher state — call before quit */
void gopher_cleanup(GopherState *gs);

/* Navigate to a Gopher URL. Returns true if connection started. */
Boolean gopher_navigate(GopherState *gs, const char *host,
    short port, char type, const char *selector);

/* Poll for data — call from event loop. Returns true if new data arrived. */
Boolean gopher_idle(GopherState *gs);

/* Parse a gopher:// URI into components.
 * Returns true on success with host, port, type, selector filled in. */
Boolean gopher_parse_uri(const char *uri, char *host, short host_size,
    short *port, char *type, char *selector, short sel_size);

/* Build a gopher:// URI from components */
void gopher_build_uri(char *uri, short uri_size, const char *host,
    short port, char type, const char *selector);

#endif /* GOPHER_H */
