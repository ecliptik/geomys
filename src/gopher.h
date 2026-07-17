/*
 * gopher.h - RFC 1436 Gopher protocol engine
 */

#ifndef GOPHER_H
#define GOPHER_H

#include "connection.h"

/* GEOMYS_MAX_WINDOWS is normally supplied by the build (-D). Fall
 * back to the single-window (minimal) assumption if it is not, so
 * the preset-aware caps below always resolve to a defined value. */
#ifndef GEOMYS_MAX_WINDOWS
#define GEOMYS_MAX_WINDOWS 1
#endif

/* Directory item array: starts small, grows dynamically */
#define GOPHER_INIT_ITEMS   128   /* initial allocation */
#define GOPHER_MAX_ITEMS   2000   /* hard cap */

/* Text buffer: starts small, grows to a preset-aware maximum.
 * GEOMYS_MAX_WINDOWS is used as a proxy for the memory partition:
 * == 1 is the minimal (512KB) preset, which gets a leaner 64KB cap;
 * larger presets have room for a 128KB cap. GOPHER_MAX_TEXT_LINES
 * must stay below 32767 because text_line_count is a short. */
#define GOPHER_TEXT_INIT_SIZE  (8L * 1024L)
#if GEOMYS_MAX_WINDOWS <= 1
#define GOPHER_TEXT_BUFSIZ     (64L * 1024L)
#define GOPHER_MAX_TEXT_LINES   4000
#else
#define GOPHER_TEXT_BUFSIZ     (128L * 1024L)
#define GOPHER_MAX_TEXT_LINES   8000
#endif

/* Text line index: starts small, grows to GOPHER_MAX_TEXT_LINES */
#define GOPHER_INIT_TEXT_LINES  512

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
	char    display[100];
	char    selector[128];
	char    host[64];
	unsigned short port;    /* 1..65535 — unsigned to hold high ports */
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
	Boolean     text_truncated; /* content hit the cap and was clipped */
	Boolean     text_line_start; /* receive parser is at a line start */

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
	unsigned short cur_port;
	char        cur_selector[256];
	char        cur_type;
	char        cur_title[128]; /* display name of current page */

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
	long        dl_parid;       /* dir ID for cleanup (System 7 FSSpec); 0 = SFReply/WD path */
	Str63       dl_filename;    /* for cleanup on error */
	short       dl_prev_page;   /* page_type before download started */
	char        dl_prev_selector[256]; /* cur_selector before download */

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
    unsigned short port, char type, const char *selector);

/* Poll for data — call from event loop. Returns true if new data arrived. */
Boolean gopher_idle(GopherState *gs);

/* Grow the text/line buffers by doubling, up to the compile-time
 * maxima. Return true on success. Shared with the HTML renderer so
 * it, too, grows the buffers instead of overflowing them. */
Boolean gopher_grow_text_buf(GopherState *gs);
Boolean gopher_grow_text_lines(GopherState *gs);

/* Parse a gopher:// URI into components.
 * Returns true on success with host, port, type, selector filled in.
 * The port is written through a short* for source compatibility with
 * existing callers; high ports (32768..65535) are stored as the
 * matching negative bit pattern and round-trip correctly through the
 * unsigned-short navigate/build_uri path. */
Boolean gopher_parse_uri(const char *uri, char *host, short host_size,
    short *port, char *type, char *selector, short sel_size);

/* Build a gopher:// URI from components */
void gopher_build_uri(char *uri, short uri_size, const char *host,
    unsigned short port, char type, const char *selector);

#ifdef GEOMYS_DEBUG
/* Format directory parsing diagnostics into buf */
void gopher_diag_str(char *buf, short buf_size);
#endif

#endif /* GOPHER_H */
