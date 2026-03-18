/*
 * content.h - Scrollable content area rendering
 */

#ifndef CONTENT_H
#define CONTENT_H

#include "gopher.h"

/* Scrollbar width (standard Mac) */
#define SCROLLBAR_WIDTH  16

/* Default row height for directory and text display (recalculated by font) */
#define ROW_HEIGHT_DEFAULT  11

/* Initialize content area — call after window and browser_init */
void content_init(WindowPtr win);

/* Clean up content area */
void content_cleanup(void);

/* Set the gopher state to render */
void content_set_page(GopherState *gs);

/* Draw visible content (clips to content area, excludes scrollbar) */
void content_draw(WindowPtr win);

/* Update scroll bar range based on content size */
void content_update_scroll(WindowPtr win);

/* Handle click in content area. Returns true if a navigation happened. */
Boolean content_click(WindowPtr win, Point local_pt, GopherState *gs);

/* Handle scroll bar click */
void content_scroll_click(WindowPtr win, Point local_pt, short part);

/* Resize — update scroll bar position and content rect */
void content_resize(WindowPtr win);

/* Get content rect (excluding scrollbar) */
void content_get_rect(WindowPtr win, Rect *r);

/* Get the scroll bar control handle */
ControlHandle content_get_scrollbar(void);

/* Scroll to top */
void content_scroll_to_top(void);

/* Update font from prefs — recalculates row height */
void content_update_font(void);

/* Get current dynamic row height */
short content_row_height(void);

/* Update cursor based on mouse position over content.
 * Call from nullEvent handler. Sets hand cursor over
 * navigable items, arrow cursor otherwise. */
void content_cursor_update(WindowPtr win, Point local_pt);

#endif /* CONTENT_H */
