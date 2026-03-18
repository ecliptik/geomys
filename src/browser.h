/*
 * browser.h - Browser chrome: nav bar, address bar, status bar
 */

#ifndef BROWSER_H
#define BROWSER_H

/* Layout constants */
#define NAV_BAR_HEIGHT    30
#define STATUS_BAR_HEIGHT 16
#define NAV_BTN_SIZE      20
#define NAV_BTN_MARGIN     3
#define NAV_BTN_Y          5   /* vertical offset within nav bar */
#define ADDR_BAR_LEFT    100   /* after 4 buttons */
#define ADDR_BAR_MARGIN   6

/* Navigation button IDs */
#define NAV_BTN_BACK      0
#define NAV_BTN_FORWARD   1
#define NAV_BTN_REFRESH   2
#define NAV_BTN_HOME      3
#define NAV_BTN_COUNT     4

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

/* Handle click in nav bar area.
 * Returns: -1 = not in chrome, -2 = address bar, -3 = disabled btn,
 *          0-3 = nav button ID clicked */
short browser_click(WindowPtr win, Point local_pt);

/* Handle keystroke in address bar. Returns true if handled. */
Boolean browser_key(WindowPtr win, EventRecord *event);

/* Activate/deactivate address bar TEHandle */
void browser_activate(Boolean active);

/* Idle for cursor blink in address bar */
void browser_idle(void);

/* Get content area rect (below nav bar, above status bar) */
void browser_get_content_rect(WindowPtr win, Rect *r);

#endif /* BROWSER_H */
