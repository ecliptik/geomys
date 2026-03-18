/*
 * favorites.h - Favorites management for Geomys
 */

#ifndef FAVORITES_H
#define FAVORITES_H

#include "settings.h"

/* Initialize favorites menu from prefs — call after prefs_load */
void favorites_init_menu(GeomysPrefs *prefs);

/* Rebuild the Favorites menu from prefs */
void favorites_rebuild_menu(GeomysPrefs *prefs);

/* Add current page as favorite */
void favorites_add(GeomysPrefs *prefs, const char *name,
    const char *url);

/* Show Manage Favorites dialog */
void favorites_manage(GeomysPrefs *prefs);

/* Handle click on a favorites menu item (dynamic entries) */
void favorites_menu_click(GeomysPrefs *prefs, short item);

#endif /* FAVORITES_H */
