/*
 * main.h - Geomys: Gopher browser for classic Macintosh
 * Global declarations, menu IDs, constants
 */

#ifndef MAIN_H
#define MAIN_H

/* Menu bar resource ID */
#define MBAR_ID             128

/* Menu resource IDs */
#define APPLE_MENU_ID       128
#define FILE_MENU_ID        129
#define EDIT_MENU_ID        130
#define FAVORITES_MENU_ID   131
#define OPTIONS_MENU_ID     132
#define FONT_MENU_ID        134

/* Apple menu items */
#define APPLE_MENU_ABOUT    1

/* File menu items */
#define FILE_MENU_OPEN_URL  1
/* separator = 2 */
#define FILE_MENU_CLOSE     3
/* separator = 4 */
#define FILE_MENU_QUIT      5

/* Edit menu items */
#define EDIT_MENU_UNDO      1
/* separator = 2 */
#define EDIT_MENU_CUT       3
#define EDIT_MENU_COPY      4
#define EDIT_MENU_PASTE     5
#define EDIT_MENU_CLEAR     6
/* separator = 7 */
#define EDIT_MENU_SELALL    8

/* Favorites menu items */
#define FAV_MENU_MANAGE     1
#define FAV_MENU_ADD        2

/* Options menu items */
#define OPT_MENU_HOME       1
/* separator = 2 */
#define OPT_MENU_FONT       3   /* hierarchical submenu trigger */

/* Font submenu items */
#define FONT_MONACO9        1
#define FONT_GENEVA9        2
#define FONT_CHICAGO12      3

/* Dialog resource IDs */
#define DLOG_ABOUT_ID       130

/* Window dimensions (Mac Plus: 512x342) */
#define SCREEN_WIDTH        512
#define SCREEN_HEIGHT       342

/* Default Gopher home page */
#define DEFAULT_HOME_HOST   "sdf.org"
#define DEFAULT_HOME_URL    "gopher://sdf.org"

/* Application state */
#define APP_STATE_IDLE      0
#define APP_STATE_LOADING   1

/* Globals (defined in main.c) */
extern Boolean g_running;
extern Boolean g_suspended;
extern WindowPtr g_window;
extern short g_app_state;

#endif /* MAIN_H */
