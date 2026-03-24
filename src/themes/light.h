/*
 * themes/light.h - Light theme (mono default)
 * White background, black text. Works on all systems.
 */

static const ThemeColors theme_light = {
	"Light",        /* name */
	0,              /* is_color */
	0,              /* is_dark */

	/* Content area */
	{ 0xFF, 0xFF, 0xFF },  /* bg: white */
	{ 0x00, 0x00, 0x00 },  /* text: black */
	{ 0x00, 0x00, 0x00 },  /* link: black */
	{ 0x00, 0x00, 0x00 },  /* link_search: black */
	{ 0x00, 0x00, 0x00 },  /* link_external: black */
	{ 0x00, 0x00, 0x00 },  /* link_download: black */
	{ 0x00, 0x00, 0x00 },  /* link_error: black */
	{ 0x00, 0x00, 0x00 },  /* label: black */
	{ 0x00, 0x00, 0x00 },  /* metadata: black */
	{ 0xEE, 0xEE, 0xEE },  /* hover_bg: light gray */

	/* Selection */
	{ 0x00, 0x00, 0x00 },  /* sel_bg: black */
	{ 0xFF, 0xFF, 0xFF },  /* sel_fg: white */

	/* Chrome */
	{ 0xFF, 0xFF, 0xFF },  /* chrome_bg: white */
	{ 0x00, 0x00, 0x00 },  /* chrome_fg: black */
	{ 0xFF, 0xFF, 0xFF },  /* addr_bg: white */
	{ 0x00, 0x00, 0x00 },  /* addr_fg: black */
};
