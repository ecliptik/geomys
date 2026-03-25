/*
 * gopherplus.h - Gopher+ protocol support
 *
 * Structures and functions for parsing Gopher+ attribute
 * responses, content negotiation, and interactive forms.
 */

#ifndef GOPHERPLUS_H
#define GOPHERPLUS_H

#ifdef GEOMYS_GOPHER_PLUS

/* Maximum views per item */
#define GPLUS_MAX_VIEWS  8

/* Admin block fields */
typedef struct {
	char    admin[80];      /* administrator name/email */
	char    mod_date[32];   /* last modification date */
} GopherPlusAdmin;

/* View entry from +VIEWS block */
typedef struct {
	char    content_type[64]; /* MIME type + optional language */
	long    size;             /* estimated size in bytes */
} GopherPlusView;

/* Complete attribute response */
typedef struct {
	GopherPlusAdmin admin;
	GopherPlusView  views[GPLUS_MAX_VIEWS];
	short           view_count;
	Boolean         has_admin;
	Boolean         has_views;
} GopherPlusInfo;

/* Initialize Gopher+ subsystem */
void gopherplus_init(void);

/* Parse +ADMIN block. Returns number of fields parsed. */
short gopherplus_parse_admin(const char *block, short block_len,
    GopherPlusAdmin *admin);

/* Parse +VIEWS block. Returns number of views found. */
short gopherplus_parse_views(const char *block, short block_len,
    GopherPlusView *views, short max_views);

/* Parse a complete Gopher+ attribute response */
void gopherplus_parse_response(const char *data, long data_len,
    GopherPlusInfo *info);

/* Fetch Gopher+ attributes for an item (synchronous).
 * Returns true on success with info filled in. */
Boolean gopherplus_fetch_info(const char *host, short port,
    const char *selector, GopherPlusInfo *info);

/* Show the Get Info dialog for the currently selected item */
void do_getinfo_dialog(void);

#endif /* GEOMYS_GOPHER_PLUS */
#endif /* GOPHERPLUS_H */
