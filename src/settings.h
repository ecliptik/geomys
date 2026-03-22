/*
 * settings.h - Preferences persistence for Geomys
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#define PREFS_VERSION    5
#define MAX_FAVORITES    20

/* Page display styles */
#define STYLE_TRADITIONAL  0  /* Monaco 9, type labels, monospaced */
#define STYLE_PLAIN        1  /* Geneva 9, underlined links, no labels */
#define STYLE_MARKDOWN     2  /* Geneva 9/Chicago 12, bullets, bold headers */

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
	short           page_style;     /* STYLE_TRADITIONAL/PLAIN/MARKDOWN */
	short           show_details;   /* 1 = show metadata columns, 0 = names only */
	/* --- v4 field: theme index (0=Light) --- */
	short           theme_id;
	/* --- v5 field: status bar visibility --- */
	short           show_status_bar;  /* 1=show (default), 0=hide */
	/* NOTE: always append new fields here, never insert above */
} GeomysPrefs;

/* Load preferences from "Geomys Preferences" file. Returns defaults if not found. */
void prefs_load(GeomysPrefs *prefs);

/* Save preferences to "Geomys Preferences" file. */
void prefs_save(GeomysPrefs *prefs);

/* Set defaults */
void prefs_defaults(GeomysPrefs *prefs);

#endif /* SETTINGS_H */
