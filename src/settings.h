/*
 * settings.h - Preferences persistence for Geomys
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#define PREFS_VERSION    8
#define MAX_FAVORITES    20

/* Page display styles — each mimics a classic Gopher client convention. */
#define STYLE_TURBOGOPHER  1  /* Mac icons (folder, doc, ?, computer, etc.) */
#define STYLE_UMN_CURSES   2  /* UMN curses: "name/" dirs, trailing <Bin>/<TEL>/etc. */
#define STYLE_XGOPHER      3  /* xgopher 1.3: "»" dirs, lowercase <bin>/<cso> prefixes */
#define STYLE_PCGOPHER     4  /* PC Gopher II: <F>/<D>/<S>/<P>/<T> brackets */
#define STYLE_RFC1436      5  /* Raw RFC 1436 type character: <0>, <1>, <9> */

typedef struct {
	char    name[64];
	char    url[256];
} GopherFavorite;

typedef struct {
	short           version;
	char            home_url[256];      /* home page URL, empty = blank */
	char            dns_server[16];     /* DNS server IP */
	short           font_id;            /* content font ID (0=Chicago) */
	short           font_size;          /* content font size (12) */
	short           favorite_count;
	GopherFavorite  favorites[MAX_FAVORITES];
	/* --- v2 fields below --- */
	short           page_style;     /* STYLE_TURBOGOPHER..STYLE_RFC1436 */
	short           show_details;   /* 1 = show metadata columns, 0 = names only */
	/* --- v4 field: theme index (0=Light) --- */
	short           theme_id;
	/* --- v5 field: status bar visibility --- */
	short           show_status_bar;  /* 1=show (default), 0=hide */
	/* --- v6 field: Gopher+ protocol support --- */
	short           gopher_plus;  /* 1=enabled, 0=disabled (default) */
	/* NOTE: always append new fields here, never insert above */
} GeomysPrefs;

/* Load preferences from "Geomys Preferences" file. Returns defaults if not found. */
void prefs_load(GeomysPrefs *prefs);

/* Save preferences to "Geomys Preferences" file. */
void prefs_save(GeomysPrefs *prefs);

/* Set defaults */
void prefs_defaults(GeomysPrefs *prefs);

#endif /* SETTINGS_H */
