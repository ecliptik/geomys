/*
 * browser.h - Browser chrome: nav bar, address bar, status bar
 */

#ifndef BROWSER_H
#define BROWSER_H

/* Focus tracking */
#define FOCUS_NONE      0
#define FOCUS_ADDR_BAR  1
#define FOCUS_CONTENT   2

/* Get/set which UI element has focus */
short browser_get_focus(void);
void browser_set_focus(short focus);

/* Layout constants */
#define NAV_BAR_HEIGHT    30
#define STATUSBAR_HEIGHT  15      /* fixed pixel height when visible */
#define NAV_BTN_SIZE      20
#define NAV_BTN_MARGIN     3
#define NAV_BTN_Y          5   /* vertical offset within nav bar */
#define ADDR_BAR_LEFT     78   /* after 3 buttons */
#define ADDR_BAR_MARGIN    6
#define ACTION_BTN_SIZE   20   /* stop/go/refresh button right of addr bar */

/* SICN resource IDs for nav button icons */
#define SICN_BACK       270
#define SICN_FORWARD    271
#define SICN_HOME       272
#define SICN_STOP       273
#define SICN_GO         274
#define SICN_REFRESH    275

/* Navigation button IDs (left-side buttons) */
#define NAV_BTN_BACK      0
#define NAV_BTN_FORWARD   1
#define NAV_BTN_HOME      2
#define NAV_BTN_COUNT     3

/* Dynamic action button states (right of address bar) */
#define ACTION_STOP       0    /* loading — click cancels */
#define ACTION_GO         1    /* idle, URL changed — click navigates */
#define ACTION_REFRESH    2    /* idle, URL same — click reloads */

/* Button states */
#define BTN_ENABLED       0
#define BTN_DISABLED      1

/* Initialize browser chrome — call after window creation */
void browser_init(WindowPtr win);

/* Clean up browser chrome */
void browser_cleanup(void);

/* Draw the full browser chrome (nav bar + status bar) */
void browser_draw(WindowPtr win);

/* Draw just the status bar with current message */
void browser_draw_status(WindowPtr win);

/* Set status bar text */
void browser_set_status(const char *msg);

/* Set address bar text */
void browser_set_url(const char *url);

/* Get address bar text into buffer */
void browser_get_url(char *buf, short buf_size);

/* Enable/disable nav buttons */
void browser_set_button_state(short btn_id, short state);

/* Action button (stop/go/refresh) */
void browser_set_action_state(short state);
short browser_get_action_state(void);

/* Handle click in nav bar area.
 * Returns: -1 = not in chrome, -2 = address bar, -3 = disabled btn,
 *          -4 = action button, 0-2 = nav button ID clicked */
short browser_click(WindowPtr win, Point local_pt);

/* Handle keystroke in address bar. Returns true if handled. */
Boolean browser_key(WindowPtr win, EventRecord *event);

/* Activate/deactivate address bar TEHandle */
void browser_activate(Boolean active);

/* Idle for cursor blink in address bar */
void browser_idle(void);

/* Update cursor to I-beam when over address bar.
 * Returns true if mouse is over address bar. */
Boolean browser_cursor_update(WindowPtr win, Point local_pt);

/* Dynamic status bar height: STATUSBAR_HEIGHT when visible, 0 when hidden */
short status_bar_height(void);

/* Get address bar rect (for drag highlight targeting) */
void browser_get_addr_rect(Rect *r);

/* Get content area rect (below nav bar, above status bar) */
void browser_get_content_rect(WindowPtr win, Rect *r);

#ifdef GEOMYS_CLIPBOARD
/* Address bar clipboard operations */
void browser_edit_cut(void);
void browser_edit_copy(void);
void browser_edit_paste(void);
void browser_edit_clear(void);
void browser_edit_select_all(void);
Boolean browser_has_selection(void);

/* Address bar undo */
void browser_edit_undo(void);
Boolean browser_can_undo(void);
Boolean browser_is_redo(void);
#endif

/* Save/restore browser chrome state to/from a session struct */
struct BrowserSession;
void browser_save_state(struct BrowserSession *s);
void browser_load_state(struct BrowserSession *s);

#endif /* BROWSER_H */
