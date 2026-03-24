/*
 * gopher_icons.c - Small bitmap icons for Gopher item types
 *
 * 11x11 pixel monochrome bitmaps, 2 bytes/row (word-aligned).
 * Drawn via CopyBits following Flynn's glyph bitmap pattern.
 *
 * Bit layout: byte 0 bit 7 = column 0 (leftmost),
 * byte 1 bits 7-5 = columns 8-10.
 */

#include <Quickdraw.h>
#include <Resources.h>
#include "gopher_icons.h"
#include "gopher.h"
#include "color.h"

/* --- Icon bitmap data (11x11, 22 bytes each) --- */

/*
 * Folder (DIR) - classic Mac folder with tab
 * ...........
 * .#####.....
 * .##########
 * .#........#
 * .#........#
 * .#........#
 * .#........#
 * .#........#
 * .#........#
 * .##########
 * ...........
 */
static const unsigned char bits_folder[] = {
	0x00,0x00, 0x7C,0x00, 0x7F,0xE0, 0x40,0x20,
	0x40,0x20, 0x40,0x20, 0x40,0x20, 0x40,0x20,
	0x40,0x20, 0x7F,0xE0, 0x00,0x00
};

/*
 * Document (TXT) - page with folded corner
 * ...........
 * ..######...
 * ..#...###..
 * ..#.....#..
 * ..#.....#..
 * ..#.....#..
 * ..#.....#..
 * ..#.....#..
 * ..#.....#..
 * ..########.
 * ...........
 */
static const unsigned char bits_document[] = {
	0x00,0x00, 0x3F,0x00, 0x21,0x80, 0x20,0x80,
	0x20,0x80, 0x20,0x80, 0x20,0x80, 0x20,0x80,
	0x20,0x80, 0x3F,0x80, 0x00,0x00
};

/*
 * Search folder (?) - folder with ? inside
 * ...........
 * .#####.....
 * .##########
 * .#..###...#
 * .#....#...#
 * .#...#....#
 * .#...#....#
 * .#........#
 * .#...#....#
 * .##########
 * ...........
 */
static const unsigned char bits_search[] = {
	0x00,0x00, 0x7C,0x00, 0x7F,0xE0, 0x4E,0x20,
	0x42,0x20, 0x44,0x20, 0x44,0x20, 0x40,0x20,
	0x44,0x20, 0x7F,0xE0, 0x00,0x00
};

/*
 * Error (ERR) - warning triangle with !
 * ...........
 * .....#.....
 * ....#.#....
 * ....#.#....
 * ...#.#.#...
 * ...#.#.#...
 * ..#.....#..
 * ..#..#..#..
 * .#.......#.
 * .#########.
 * ...........
 */
static const unsigned char bits_error[] = {
	0x00,0x00, 0x04,0x00, 0x0A,0x00, 0x0A,0x00,
	0x15,0x00, 0x15,0x00, 0x20,0x80, 0x24,0x80,
	0x40,0x40, 0x7F,0xC0, 0x00,0x00
};

/*
 * Binary (HQX/DOS/UUE/BIN) - box with divider
 * ...........
 * .#########.
 * .#.......#.
 * .#.......#.
 * .#.......#.
 * .#########.
 * .#.......#.
 * .#.......#.
 * .#.......#.
 * .#########.
 * ...........
 */
static const unsigned char bits_binary[] = {
	0x00,0x00, 0x7F,0xC0, 0x40,0x40, 0x40,0x40,
	0x40,0x40, 0x7F,0xC0, 0x40,0x40, 0x40,0x40,
	0x40,0x40, 0x7F,0xC0, 0x00,0x00
};

/*
 * Terminal (TEL/3270) - monitor with cursor and base
 * ...........
 * .#########.
 * .#.......#.
 * .#.#.....#.
 * .#.......#.
 * .#.......#.
 * .#########.
 * .....#.....
 * ...#####...
 * ...........
 * ...........
 */
static const unsigned char bits_terminal[] = {
	0x00,0x00, 0x7F,0xC0, 0x40,0x40, 0x50,0x40,
	0x40,0x40, 0x40,0x40, 0x7F,0xC0, 0x04,0x00,
	0x1F,0x00, 0x00,0x00, 0x00,0x00
};

/*
 * Image (GIF/IMG/PNG) - picture frame with sun & mountain
 * ...........
 * .#########.
 * .#.......#.
 * .#.#.....#.
 * .#.......#.
 * .#.......#.
 * .#....#..#.
 * .#...#.#.#.
 * .#..#...##.
 * .#########.
 * ...........
 */
static const unsigned char bits_image[] = {
	0x00,0x00, 0x7F,0xC0, 0x40,0x40, 0x50,0x40,
	0x40,0x40, 0x40,0x40, 0x44,0x40, 0x4A,0x40,
	0x51,0x40, 0x7F,0xC0, 0x00,0x00
};

/*
 * Globe (HTM) - circle with meridian and equator
 * ...........
 * ...#####...
 * ..#..#..#..
 * .#...#...#.
 * .#...#...#.
 * .#########.
 * .#...#...#.
 * .#...#...#.
 * ..#..#..#..
 * ...#####...
 * ...........
 */
static const unsigned char bits_globe[] = {
	0x00,0x00, 0x1F,0x00, 0x24,0x80, 0x44,0x40,
	0x44,0x40, 0x7F,0xC0, 0x44,0x40, 0x44,0x40,
	0x24,0x80, 0x1F,0x00, 0x00,0x00
};

/*
 * Speaker (SND) - cone with sound waves
 * ...........
 * .....#.....
 * ....##.....
 * ...###..#..
 * ..####.....
 * .#####.#.#.
 * ..####.....
 * ...###..#..
 * ....##.....
 * .....#.....
 * ...........
 */
static const unsigned char bits_speaker[] = {
	0x00,0x00, 0x04,0x00, 0x0C,0x00, 0x1C,0x80,
	0x3C,0x00, 0x7D,0x40, 0x3C,0x00, 0x1C,0x80,
	0x0C,0x00, 0x04,0x00, 0x00,0x00
};

/*
 * Phonebook (CSO) - book with ruled entries
 * ...........
 * .########..
 * .#......#..
 * .#.####.#..
 * .#......#..
 * .#.####.#..
 * .#......#..
 * .#.####.#..
 * .#......#..
 * .########..
 * ...........
 */
static const unsigned char bits_phonebook[] = {
	0x00,0x00, 0x7F,0x80, 0x40,0x80, 0x5E,0x80,
	0x40,0x80, 0x5E,0x80, 0x40,0x80, 0x5E,0x80,
	0x40,0x80, 0x7F,0x80, 0x00,0x00
};

/*
 * Unknown (fallback) - box with ? inside
 * ...........
 * .#########.
 * .#..###..#.
 * .#....#..#.
 * .#...#...#.
 * .#...#...#.
 * .#.......#.
 * .#...#...#.
 * .#.......#.
 * .#########.
 * ...........
 */
static const unsigned char bits_unknown[] = {
	0x00,0x00, 0x7F,0xC0, 0x4E,0x40, 0x42,0x40,
	0x44,0x40, 0x44,0x40, 0x40,0x40, 0x44,0x40,
	0x40,0x40, 0x7F,0xC0, 0x00,0x00
};

/* --- Icon table --- */

enum {
	ICON_FOLDER,
	ICON_DOCUMENT,
	ICON_SEARCH,
	ICON_ERROR,
	ICON_BINARY,
	ICON_TERMINAL,
	ICON_IMAGE,
	ICON_GLOBE,
	ICON_SPEAKER,
	ICON_PHONEBOOK,
	ICON_UNKNOWN,
	ICON_COUNT
};

static const GopherIcon icon_table[ICON_COUNT] = {
	{ 11, 11, 2, bits_folder },
	{ 11, 11, 2, bits_document },
	{ 11, 11, 2, bits_search },
	{ 11, 11, 2, bits_error },
	{ 11, 11, 2, bits_binary },
	{ 11, 11, 2, bits_terminal },
	{ 11, 11, 2, bits_image },
	{ 11, 11, 2, bits_globe },
	{ 11, 11, 2, bits_speaker },
	{ 11, 11, 2, bits_phonebook },
	{ 11, 11, 2, bits_unknown }
};

const GopherIcon *
gopher_icon_for_type(char type)
{
	switch (type) {
	case GOPHER_TEXT:
		return &icon_table[ICON_DOCUMENT];
	case GOPHER_DIRECTORY:
		return &icon_table[ICON_FOLDER];
	case GOPHER_CSO:
		return &icon_table[ICON_PHONEBOOK];
	case GOPHER_ERROR:
		return &icon_table[ICON_ERROR];
	case GOPHER_BINHEX:
	case GOPHER_DOS:
	case GOPHER_UUENCODE:
	case GOPHER_BINARY:
		return &icon_table[ICON_BINARY];
	case GOPHER_SEARCH:
		return &icon_table[ICON_SEARCH];
	case GOPHER_TELNET:
	case GOPHER_TN3270:
		return &icon_table[ICON_TERMINAL];
	case GOPHER_GIF:
	case GOPHER_IMAGE:
	case GOPHER_PNG:
		return &icon_table[ICON_IMAGE];
	case GOPHER_DOC:
	case GOPHER_RTF:
		return &icon_table[ICON_DOCUMENT];
	case GOPHER_HTML:
		return &icon_table[ICON_GLOBE];
	case GOPHER_INFO:
		return (const GopherIcon *)0L;
	case GOPHER_SOUND:
		return &icon_table[ICON_SPEAKER];
	default:
		return &icon_table[ICON_UNKNOWN];
	}
}

void
gopher_icon_draw(const GopherIcon *icon, short x, short y,
    short invert)
{
	BitMap src_bits;
	Rect src_r, dst_r;

	if (!icon)
		return;

	src_bits.baseAddr = (Ptr)icon->bits;
	src_bits.rowBytes = icon->rowBytes;
	SetRect(&src_bits.bounds, 0, 0,
	    icon->width, icon->height);
	SetRect(&src_r, 0, 0,
	    icon->width, icon->height);
	SetRect(&dst_r, x, y,
	    x + icon->width, y + icon->height);

	CopyBits(&src_bits, &qd.thePort->portBits,
	    &src_r, &dst_r,
	    invert ? srcBic : srcOr, NULL);
}

/* --- SICN (16x16) icon support --- */

short
gopher_sicn_for_type(char type)
{
	switch (type) {
	case GOPHER_TEXT:
		return SICN_DOCUMENT;
	case GOPHER_DIRECTORY:
		return SICN_FOLDER;
	case GOPHER_CSO:
		return SICN_PHONEBOOK;
	case GOPHER_ERROR:
		return SICN_ERROR;
	case GOPHER_BINHEX:
	case GOPHER_DOS:
	case GOPHER_UUENCODE:
	case GOPHER_BINARY:
		return SICN_BINARY;
	case GOPHER_SEARCH:
		return SICN_SEARCH;
	case GOPHER_TELNET:
	case GOPHER_TN3270:
		return SICN_TERMINAL;
	case GOPHER_GIF:
	case GOPHER_IMAGE:
	case GOPHER_PNG:
		return SICN_IMAGE;
	case GOPHER_DOC:
	case GOPHER_RTF:
		return SICN_DOCUMENT;
	case GOPHER_HTML:
		return SICN_GLOBE;
	case GOPHER_INFO:
		return 0;
	case GOPHER_SOUND:
		return SICN_SPEAKER;
	default:
		return SICN_UNKNOWN;
	}
}

void
gopher_sicn_draw(short sicn_id, short x, short y,
    short invert)
{
	Handle h;
	BitMap src_bits;
	Rect src_r, dst_r;

	if (sicn_id == 0)
		return;

	h = GetResource('SICN', sicn_id);
	if (!h)
		return;

	/* Lock handle — CopyBits reads from the pointer,
	 * which must not move during the blit. */
	HLock(h);

	/* SICN data: 16x16, 2 bytes/row, 32 bytes total.
	 * First icon in the SICN list starts at offset 0. */
	src_bits.baseAddr = *h;
	src_bits.rowBytes = 2;
	SetRect(&src_bits.bounds, 0, 0, 16, 16);
	SetRect(&src_r, 0, 0, 16, 16);
	SetRect(&dst_r, x, y, x + 16, y + 16);

	CopyBits(&src_bits, &qd.thePort->portBits,
	    &src_r, &dst_r,
	    invert ? srcBic : srcOr, NULL);

	HUnlock(h);
}

/* --- cicn (color icon) support --- */

#ifdef GEOMYS_COLOR

/* Cache covers SICN/cicn IDs 256-275 (Gopher types + nav) */
#define CICN_BASE_ID    256
#define CICN_CACHE_SIZE  20

static CIconHandle g_cicn_cache[CICN_CACHE_SIZE];
static unsigned char g_cicn_tried[CICN_CACHE_SIZE];

static CIconHandle
cicn_get_cached(short cicn_id)
{
	short idx;

	idx = cicn_id - CICN_BASE_ID;
	if (idx < 0 || idx >= CICN_CACHE_SIZE)
		return NULL;

	if (!g_cicn_tried[idx]) {
		g_cicn_tried[idx] = 1;
		g_cicn_cache[idx] = GetCIcon(cicn_id);
	}
	return g_cicn_cache[idx];
}

unsigned char
gopher_cicn_draw(short sicn_id, short x, short y)
{
	CIconHandle cic;
	Rect dst_r;

	if (!g_has_color_qd || sicn_id == 0)
		return 0;

	cic = cicn_get_cached(sicn_id);
	if (!cic)
		return 0;

	SetRect(&dst_r, x, y, x + 16, y + 16);
	PlotCIcon(&dst_r, cic);
	return 1;
}

void
gopher_cicn_cleanup(void)
{
	short i;

	for (i = 0; i < CICN_CACHE_SIZE; i++) {
		if (g_cicn_cache[i]) {
			DisposeCIcon(g_cicn_cache[i]);
			g_cicn_cache[i] = NULL;
			g_cicn_tried[i] = 0;
		}
	}
}

#endif /* GEOMYS_COLOR */
