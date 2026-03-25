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
	char            abstract[256];  /* +ABSTRACT text */
	short           view_count;
	short           score;          /* 0-100 or -1 */
	Boolean         has_admin;
	Boolean         has_views;
	Boolean         has_abstract;
	Boolean         has_score;
	Boolean         has_ask;
	void            *ask_form; /* GopherPlusAskForm*, caller frees */
} GopherPlusInfo;

/* +ASK field types */
#define ASK_TYPE_ASK       0   /* single line text */
#define ASK_TYPE_ASKP      1   /* password (masked) */
#define ASK_TYPE_ASKL      2   /* multi-line text */
#define ASK_TYPE_CHOOSE    3   /* radio buttons */
#define ASK_TYPE_SELECT    4   /* checkbox */

#define GPLUS_ASK_MAX_FIELDS   8
#define GPLUS_ASK_MAX_CHOICES  8

/* Single +ASK form field (~486 bytes) */
typedef struct {
	short   type;
	char    prompt[80];
	char    default_val[80];
	char    choices[GPLUS_ASK_MAX_CHOICES][40];
	short   choice_count;
	short   selected;
} GopherPlusAskField;

/* Complete +ASK form (~3.9 KB — MUST be heap-allocated) */
typedef struct {
	GopherPlusAskField fields[GPLUS_ASK_MAX_FIELDS];
	short   field_count;
} GopherPlusAskForm;

/* Initialize Gopher+ subsystem */
void gopherplus_init(void);

/* Parse +ADMIN block. Returns number of fields parsed. */
short gopherplus_parse_admin(const char *block, short block_len,
    GopherPlusAdmin *admin);

/* Parse +VIEWS block. Returns number of views found. */
short gopherplus_parse_views(const char *block, short block_len,
    GopherPlusView *views, short max_views);

/* Parse +SCORE block. Returns 0-100 or -1 on failure. */
short gopherplus_parse_score(const char *block, short block_len);

/* Parse +ABSTRACT block. Returns length of parsed abstract. */
short gopherplus_parse_abstract(const char *block, short block_len,
    char *abstract, short max_len);

/* Parse a complete Gopher+ attribute response */
void gopherplus_parse_response(const char *data, long data_len,
    GopherPlusInfo *info);

/* Fetch Gopher+ attributes for an item (synchronous).
 * Returns true on success with info filled in. */
Boolean gopherplus_fetch_info(const char *host, short port,
    const char *selector, GopherPlusInfo *info);

/* Show the Get Info dialog for the currently selected item */
void do_getinfo_dialog(void);

/* Show view selection dialog. Returns chosen view index (0-based)
 * or -1 if cancelled. */
short do_view_select_dialog(GopherPlusInfo *info);

/* Parse +ASK block into a heap-allocated form.
 * Returns field count, 0 on failure. */
short gopherplus_parse_ask(const char *block, short block_len,
    GopherPlusAskForm *form);

/* Show +ASK form dialog. Returns 0 on Send, -1 on Cancel.
 * Form field values are updated in-place on Send. */
short do_ask_dialog(GopherPlusAskForm *form, const char *title);

/* --- Bulk attribute cache --- */

#define GPLUS_CACHE_MAX   16     /* max cached entries per directory */
#define GPLUS_BULK_BUFSIZ 8192L  /* response buffer for $ request */

/* Compact per-item cache entry (~198 bytes) */
typedef struct {
	char    selector[128];   /* item selector for matching */
	char    abstract[64];    /* truncated abstract text */
	short   score;           /* 0-100 or -1 */
	Boolean has_abstract;
	Boolean has_score;
	Boolean has_ask;
} GopherPlusCacheEntry;

/* Directory attribute cache */
typedef struct {
	GopherPlusCacheEntry entries[GPLUS_CACHE_MAX];
	short   count;           /* number of valid entries */
	Boolean fetched;         /* true after bulk fetch attempted */
} GopherPlusCache;

/* Fetch bulk attributes for current directory (selector\t$).
 * Populates cache. Returns true on success. */
Boolean gopherplus_fetch_bulk(const char *host, short port,
    const char *selector, GopherPlusCache *cache);

/* Look up cached attributes for a selector.
 * Returns pointer to entry or NULL. */
const GopherPlusCacheEntry *gopherplus_cache_lookup(
    const GopherPlusCache *cache, const char *selector);

/* Clear the cache */
void gopherplus_cache_clear(GopherPlusCache *cache);

#endif /* GEOMYS_GOPHER_PLUS */
#endif /* GOPHERPLUS_H */
