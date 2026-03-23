/*
 * themes/platinum.h - Platinum theme
 * Mac OS 8/9 Platinum Appearance.
 * Gray background, purple accent links, lavender highlights.
 * Colors chosen to align with 256-color palette (6x6x6 cube).
 */

static const ThemeColors theme_platinum = {
	"Platinum",     /* name */
	1,              /* is_color */
	0,              /* is_dark */

	/* Content area — Mac OS 8/9 inspired */
	{ 0xCC, 0xCC, 0xCC },  /* bg: Platinum gray */
	{ 0x00, 0x00, 0x00 },  /* text: black */
	{ 0x00, 0x00, 0xCC },  /* link: Mac blue #0000CC */
	{ 0x00, 0x66, 0x66 },  /* link_search: teal #006666 */
	{ 0x66, 0x00, 0xCC },  /* link_external: purple #6600CC */
	{ 0xCC, 0x00, 0x00 },  /* link_error: red */
	{ 0x66, 0x66, 0x99 },  /* label: muted purple-gray #666699 */
	{ 0x66, 0x66, 0x66 },  /* metadata: gray */
	{ 0x99, 0x99, 0xFF },  /* hover_bg: lavender #9999FF */

	/* Selection */
	{ 0x99, 0x99, 0xFF },  /* sel_bg: lavender #9999FF */
	{ 0x00, 0x00, 0x00 },  /* sel_fg: black */

	/* Chrome */
	{ 0xCC, 0xCC, 0xCC },  /* chrome_bg: Platinum gray (matches content bg) */
	{ 0x00, 0x00, 0x00 },  /* chrome_fg: black */
	{ 0xFF, 0xFF, 0xFF },  /* addr_bg: white */
	{ 0x00, 0x00, 0x00 },  /* addr_fg: black */
};
