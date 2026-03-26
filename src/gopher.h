/*
 * gopher.h - RFC 1436 Gopher protocol engine
 */

#ifndef GOPHER_H
#define GOPHER_H

#include "connection.h"

/* Directory item array: starts small, grows dynamically */
#define GOPHER_INIT_ITEMS    64   /* initial allocation */
#define GOPHER_MAX_ITEMS   2000   /* hard cap */

/* Text buffer: starts small, grows to max */
#define GOPHER_TEXT_INIT_SIZE  (8L * 1024L)
#define GOPHER_TEXT_BUFSIZ     (32L * 1024L)

/* Text line index: starts small, grows to max */
#define GOPHER_INIT_TEXT_LINES  512
#define GOPHER_MAX_TEXT_LINES   3000

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
#define PAGE_DOWNLOAD       4
#define PAGE_IMAGE          5
#ifdef GEOMYS_HTML
#define PAGE_HTML           6
#endif

typedef struct {
	char    type;
	char    display[80];
	char    selector[256];
	char    host[64];
	short   port;
#ifdef GEOMYS_GOPHER_PLUS
	short   score;          /* search relevance 0-100, -1 = none */
	char    has_plus;       /* 1 if server supports Gopher+ for this item */
#endif
} GopherItem;

typedef struct {
	/* Current page content */
	short       page_type;      /* PAGE_DIRECTORY, PAGE_TEXT, etc. */

	/* Directory listing */
	GopherItem  *items;         /* NewPtr-allocated, grows dynamically */
	short       item_count;
	short       item_capacity;  /* current allocation size */

	/* Text content */
	char        *text_buf;      /* NewPtr-allocated, starts GOPHER_TEXT_INIT_SIZE */
	long        text_len;
	long        text_buf_capacity;  /* current allocation size */
	long        *text_lines;    /* byte offsets of each line start */
	short       text_line_count;
	short       text_lines_capacity; /* current line index allocation */

	/* Connection */
	Connection  conn;

	/* Parse state for incremental receive */
	char        line_buf[512];
	short       line_len;
	Boolean     selector_sent;
	Boolean     receiving;
	short       cso_last_entry;  /* CSO entry boundary tracker */

	/* Current request */
	char        cur_host[64];
	short       cur_port;
	char        cur_selector[256];
	char        cur_type;
	char        cur_title[80];  /* display name of current page */

#ifdef GEOMYS_GOPHER_PLUS
	void        *gplus_cache;   /* GopherPlusCache*, heap-allocated */
	char        gplus_view[64]; /* Gopher+ view MIME to request */
	void        *gplus_ask_form; /* GopherPlusAskForm* for +ASK submit */
	Boolean     gplus_active;   /* Gopher+ content request active */
	Boolean     gplus_status_parsed;  /* status line consumed */
#endif

#ifdef GEOMYS_DOWNLOAD
	/* Download state (PAGE_DOWNLOAD / PAGE_IMAGE) */
	short       dl_refnum;      /* open file refNum, 0 = not open */
	long        dl_written;     /* bytes written so far */
	Boolean     dl_error;       /* sticky write error flag */
	short       dl_vrefnum;     /* volume for cleanup on error */
	Str63       dl_filename;    /* for cleanup on error */
	short       dl_prev_page;   /* page_type before download started */

	/* Image sniff state (PAGE_IMAGE) */
	char        img_header[26]; /* enough for GIF or PNG header */
	short       img_header_len; /* bytes collected so far */
	Boolean     img_sniffed;    /* header parsed, waiting for user */
#endif

#ifdef GEOMYS_HTML
	/* HTML tag-stripping parser state */
	short       html_state;         /* TEXT=0, TAG_OPEN=1, ENTITY=2, SKIP=3 */
	char        html_tag[16];       /* current tag name accumulator */
	short       html_tag_len;
	char        html_entity[8];     /* entity accumulator */
	short       html_ent_len;
	Boolean     html_in_pre;        /* inside <pre> block */
	Boolean     html_had_space;     /* whitespace collapse flag */
	char        html_in_skip_tag[8]; /* tag name being skipped (script/style) */
#endif
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
