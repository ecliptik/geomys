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
#define WINDOW_MENU_ID      133
#define FONT_MENU_ID        134
#define STYLE_MENU_ID       135
#define THEME_MENU_ID       136

/* Window menu items (dynamic: items 3+ are window list) */
#define WIN_MENU_HEADER     1
/* separator = 2 */
#define WIN_MENU_FIRST_WIN  3

/* Apple menu items */
#define APPLE_MENU_ABOUT    1

/* File menu items */
#define FILE_MENU_NEW_WIN   1
/* separator = 2 */
#define FILE_MENU_SAVE_AS   3
/* separator = 4 */
#define FILE_MENU_CLOSE     5
/* separator = 6 */
#define FILE_MENU_QUIT      7

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
#define OPT_MENU_STYLE      4   /* hierarchical submenu trigger */
#define OPT_MENU_THEME      5   /* hierarchical submenu trigger */
/* separator = 6 */
#define OPT_MENU_DETAILS    7   /* "Show Details" toggle */
#define OPT_MENU_STATUS_BAR 8   /* "Status Bar" toggle */

/* Font submenu items */
#define FONT_MONACO9        1
#define FONT_MONACO12       2
#define FONT_CHICAGO12      3
#define FONT_COURIER10      4
#define FONT_GENEVA9        5
#define FONT_GENEVA10       6

/* Page Style submenu items */
#define STYLE_ITEM_TRADITIONAL  1
#define STYLE_ITEM_PLAIN        2
#define STYLE_ITEM_MARKDOWN     3

/* Theme submenu items */
#define THEME_ITEM_LIGHT           1
#define THEME_ITEM_DARK            2
/* separator = 3 */
#define THEME_ITEM_SOLARIZED_L     4
#define THEME_ITEM_SOLARIZED_D     5
#define THEME_ITEM_TOKYO_L         6
#define THEME_ITEM_TOKYO_D         7
#define THEME_ITEM_GREEN           8
#define THEME_ITEM_CLASSIC         9
#define THEME_ITEM_PLATINUM        10
#define THEME_ITEM_LAST            10

/* Dialog resource IDs */
#define DLOG_ABOUT_ID       130
#define DLOG_SEARCH_ID      132
#define DLOG_HOME_PAGE_ID   133
#define DLOG_FAVORITES_ID   134
#define DLOG_EDIT_FAV_ID    135

/* Window dimensions (Mac Plus: 512x342) */
#define SCREEN_WIDTH        512
#define SCREEN_HEIGHT       342

/* Default Gopher home page */
#define DEFAULT_HOME_HOST   "sdf.org"
#define DEFAULT_HOME_URL    "gopher://sdf.org"

/* Application state */
#define APP_STATE_IDLE      0
#define APP_STATE_LOADING   1

/* Functions callable from menus and content click handlers */
void do_new_window(void);
void do_navigate_url_titled(const char *url, const char *title);
void do_navigate_url(const char *url);  /* title = NULL */
void do_search_dialog(const char *title, const char *host,
    short port, const char *selector);
void do_type_message(char type, const char *display,
    const char *host, short port);
void do_home_page_dialog(void);

/* Globals (defined in main.c) */
extern Boolean g_running;
extern Boolean g_suspended;

/*
 * Per-session state accessed through active_session.
 * For GEOMYS_MAX_WINDOWS == 1 these are direct struct access
 * with zero overhead (active_session is a compile-time macro).
 */
#include "session.h"
#define g_window     (active_session->window)
#define g_app_state  (active_session->app_state)
#define g_gopher     (active_session->gopher)

#endif /* MAIN_H */
