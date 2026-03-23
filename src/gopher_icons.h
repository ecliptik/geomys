/*
 * gopher_icons.h - Small bitmap icons for Gopher item types
 *
 * Each icon is 11x11 pixels, drawn via CopyBits for use in
 * the Icons page style. Inspired by TurboGopher's icon view.
 */

#ifndef GOPHER_ICONS_H
#define GOPHER_ICONS_H

typedef struct {
	short               width;      /* 11 */
	short               height;    /* 11 */
	short               rowBytes;   /* 2 (word-aligned) */
	const unsigned char *bits;      /* 22 bytes per icon */
} GopherIcon;

/* Get the icon bitmap for a Gopher item type character.
 * Returns NULL for type 'i' (info) or if no icon defined. */
const GopherIcon *gopher_icon_for_type(char type);

/* Draw a Gopher type icon at the specified position.
 * x,y is the top-left corner of the icon area.
 * invert: use srcBic instead of srcOr (for inverse video). */
void gopher_icon_draw(const GopherIcon *icon, short x, short y,
    short invert);

#endif /* GOPHER_ICONS_H */
