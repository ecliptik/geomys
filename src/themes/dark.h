/*
 * themes/dark.h - Dark theme (mono)
 * Black background, white text. Works on all systems.
 */

static const ThemeColors theme_dark = {
	"Dark",         /* name */
	0,              /* is_color */
	1,              /* is_dark */

	/* Content area */
	{ 0x00, 0x00, 0x00 },  /* bg: black */
	{ 0xFF, 0xFF, 0xFF },  /* text: white */
	{ 0xFF, 0xFF, 0xFF },  /* link: white */
	{ 0xFF, 0xFF, 0xFF },  /* link_search: white */
	{ 0xFF, 0xFF, 0xFF },  /* link_external: white */
	{ 0xFF, 0xFF, 0xFF },  /* link_error: white */
	{ 0xFF, 0xFF, 0xFF },  /* label: white */
	{ 0xFF, 0xFF, 0xFF },  /* metadata: white */
	{ 0x22, 0x22, 0x22 },  /* hover_bg: dark gray */

	/* Selection */
	{ 0xFF, 0xFF, 0xFF },  /* sel_bg: white */
	{ 0x00, 0x00, 0x00 },  /* sel_fg: black */

	/* Chrome */
	{ 0x00, 0x00, 0x00 },  /* chrome_bg: black */
	{ 0xFF, 0xFF, 0xFF },  /* chrome_fg: white */
	{ 0x00, 0x00, 0x00 },  /* addr_bg: black */
	{ 0xFF, 0xFF, 0xFF },  /* addr_fg: white */
};
