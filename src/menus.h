/*
 * menus.h - Menu management for Geomys
 */

#ifndef MENUS_H
#define MENUS_H

void init_menus(void);
void update_menus(void);
Boolean handle_menu(long menu_id);

/* SICN menu icon support (System 7+).
 * Uses SetItemCmd(0x1E) per Inside Macintosh, so only works
 * on items WITHOUT keyboard shortcuts. */
unsigned char menu_has_icons(void);
void menu_set_item_sicn(MenuHandle menu, short item,
    short sicn_id);

/* Clipboard window (HIG p.112: document window, scrollable,
 * non-editable). Used by main event loop for update/close. */
WindowPtr clipboard_window_ptr(void);
void clipboard_window_update(WindowPtr win);
void clipboard_window_close(void);
void clipboard_window_click(WindowPtr win, Point where);
void clipboard_window_grow(WindowPtr win, Point where);

#endif /* MENUS_H */
