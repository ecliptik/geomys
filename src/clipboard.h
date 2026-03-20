/*
 * clipboard.h - Clipboard and text selection for Geomys
 */

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#ifdef GEOMYS_CLIPBOARD
/* Copy selected content area text to system clipboard */
void clipboard_copy_content(WindowPtr win);

/* Select all rows in content area */
void clipboard_select_all(WindowPtr win);
#else
#define clipboard_copy_content(w) ((void)0)
#define clipboard_select_all(w)   ((void)0)
#endif

#endif /* CLIPBOARD_H */
