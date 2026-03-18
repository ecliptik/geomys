/*
 * settings.h - Preferences persistence for Geomys
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#define PREFS_VERSION    1
#define MAX_FAVORITES    20

typedef struct {
	char    name[64];
	char    url[256];
} GopherFavorite;

typedef struct {
	short           version;
	char            home_url[256];      /* home page URL, empty = blank */
	char            dns_server[16];     /* DNS server IP */
	short           font_id;            /* content font ID (4=Monaco) */
	short           font_size;          /* content font size (9) */
	short           favorite_count;
	GopherFavorite  favorites[MAX_FAVORITES];
	/* NOTE: always append new fields here, never insert above */
} GeomysPrefs;

/* Load preferences from "Geomys Preferences" file. Returns defaults if not found. */
void prefs_load(GeomysPrefs *prefs);

/* Save preferences to "Geomys Preferences" file. */
void prefs_save(GeomysPrefs *prefs);

/* Set defaults */
void prefs_defaults(GeomysPrefs *prefs);

#endif /* SETTINGS_H */
