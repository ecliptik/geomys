/*
 * content.h - Scrollable content area rendering
 */

#ifndef CONTENT_H
#define CONTENT_H

#include "gopher.h"

#ifdef GEOMYS_CLIPBOARD
/* Text selection state */
typedef struct {
	short		active;
	short		selecting;
	short		anchor_row;
	short		anchor_col;
	short		extent_row;
	short		extent_col;
	short		word_mode;
	short		word_anchor_start;
	short		word_anchor_end;
	unsigned long	last_click_ticks;
	short		last_click_row;
	short		last_click_col;
} Selection;
#endif

/* Scrollbar width (standard Mac) */
#define SCROLLBAR_WIDTH  15

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

/* Get the scroll bar control handles */
ControlHandle content_get_scrollbar(void);
ControlHandle content_get_hscrollbar(void);

/* Handle horizontal scroll bar click */
void content_hscroll_click(WindowPtr win, Point local_pt, short part);

/* Scroll horizontally by delta pixels (positive=right, negative=left) */
void content_hscroll_by(short delta);

/* Scroll to absolute horizontal pixel position */
void content_hscroll_to(short pos);

/* Get one character-width horizontal scroll step in pixels */
short content_hscroll_step(void);

/* Scroll to top */
void content_scroll_to_top(void);

/* Get current scroll position (first visible row) */
short content_get_scroll_pos(void);

/* Set scroll position — clamps, updates scrollbar, redraws */
void content_set_scroll_pos(short pos);

/* Scroll vertically by delta rows (positive=down, negative=up) */
void content_vscroll_by(short delta);

/* Get number of rows visible in content area */
short content_visible_rows(void);

/* Update font from prefs — recalculates row height */
void content_update_font(void);

/* Recalculate max content width for horizontal scrollbar */
void content_recalc_width(WindowPtr win);

/* Get current dynamic row height */
short content_row_height(void);

/* Update cursor based on mouse position over content.
 * Call from nullEvent handler. Sets hand cursor over
 * navigable items, arrow cursor otherwise. */
void content_cursor_update(WindowPtr win, Point local_pt);

#ifdef GEOMYS_CLIPBOARD
/* Find in page — case-insensitive substring search.
 * Returns true if match found, scrolls to and highlights it. */
Boolean content_find(const char *query);

/* Repeat last find from current match position + 1 */
Boolean content_find_again(void);

/* Returns true if a find query is active (for Find Again enable) */
Boolean content_find_active(void);

/* Get the current find query (for dialog pre-fill) */
const char *content_find_query(void);

/* Activate/deactivate content selection display */
void content_activate(WindowPtr win, Boolean active);

/* Check if content has active selection */
Boolean content_has_selection(void);

/* Get display text for a row (formatted with style prefix for
 * directory, raw line for text pages). Returns length. */
short content_row_text(short row, char *buf, short bufsiz);

/* Get normalized selection range (start <= end in reading order).
 * Returns false if no selection. */
Boolean content_get_selection(short *start_row, short *start_col,
    short *end_row, short *end_col);

/* Select all rows in content, marking selection active */
void content_select_all(WindowPtr win);

/* Clear content selection and redraw affected rows */
void content_clear_selection(WindowPtr win);
#endif

/* Dirty-row tracking: mark rows needing redraw */
void content_mark_dirty(short row);
void content_mark_all_dirty(void);

/* Save/restore content area state to/from a session struct */
struct BrowserSession;
void content_save_state(struct BrowserSession *s);
void content_load_state(struct BrowserSession *s);

#endif /* CONTENT_H */
