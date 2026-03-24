/*
 * html.h - HTML tag-stripping renderer for Gopher type h
 */

#ifndef HTML_H
#define HTML_H

#ifdef GEOMYS_HTML

#include "gopher.h"

/* Initialize HTML parser state — call before processing */
void html_init(GopherState *gs);

/* Process incoming data through the HTML tag stripper.
 * Writes clean text into gs->text_buf, updates text_lines[]. */
void html_process_data(GopherState *gs, const char *buf, long len);

#endif /* GEOMYS_HTML */

#endif /* HTML_H */
