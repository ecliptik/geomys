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

/* SICN (16x16) icon support for larger font sizes */

/* SICN resource IDs for Gopher types */
#define SICN_FOLDER     256
#define SICN_DOCUMENT   257
#define SICN_SEARCH     258
#define SICN_ERROR      259
#define SICN_BINARY     260
#define SICN_TERMINAL   261
#define SICN_IMAGE      262
#define SICN_GLOBE      263
#define SICN_SPEAKER    264
#define SICN_PHONEBOOK  265
#define SICN_UNKNOWN    266

/* Get the SICN resource ID for a Gopher item type.
 * Returns 0 for type 'i' (info) or if no icon defined. */
short gopher_sicn_for_type(char type);

/* Draw a 16x16 SICN icon at the specified position.
 * Loads the SICN resource by ID and draws via CopyBits.
 * invert: use srcBic instead of srcOr (for inverse video). */
void gopher_sicn_draw(short sicn_id, short x, short y,
    short invert);

#ifdef GEOMYS_COLOR
/* cicn (color icon) support for Color QuickDraw systems.
 * Uses the same resource IDs as SICN (256-275). */

/* Draw a 16x16 cicn color icon at the specified position.
 * Returns 1 if drawn successfully, 0 if cicn not available
 * (caller should fall back to SICN). */
unsigned char gopher_cicn_draw(short sicn_id, short x,
    short y);

/* Dispose all cached cicn handles (call at cleanup). */
void gopher_cicn_cleanup(void);
#endif

#endif /* GOPHER_ICONS_H */
