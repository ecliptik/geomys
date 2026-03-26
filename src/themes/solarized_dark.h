/*
 * themes/solarized_dark.h - Solarized Dark theme
 * Ethan Schoonover's Solarized palette, dark variant.
 * https://ethanschoonover.com/solarized/
 * Each color is the canonical Solarized hex mapped to nearest 6x6x6 cube.
 */

static const ThemeColors theme_solarized_dark = {
	"Solarized Dark",   /* name */
	1,                  /* is_color */
	1,                  /* is_dark */

	/* Content area — colors brightened from canonical Solarized
	 * for readable contrast on 256-color CRT displays */
	{ 0x00, 0x33, 0x33 },  /* bg: base03 #002B36 → #003333 */
	{ 0xCC, 0xCC, 0xCC },  /* text: base0 #839496 → #CCCCCC (8.6:1) */
	{ 0x66, 0xCC, 0xFF },  /* link: blue #268BD2 → #66CCFF (7.7:1) */
	{ 0x66, 0xCC, 0xCC },  /* link_search: cyan #2AA198 → #66CCCC (7.3:1) */
	{ 0xFF, 0x99, 0x33 },  /* link_external: orange #CB4B16 → #FF9933 (6.5:1) */
	{ 0x99, 0xCC, 0x33 },  /* link_download: green #859900 → #99CC33 (7.3:1) */
	{ 0xFF, 0x66, 0x66 },  /* link_error: red #DC322F → #FF6666 (4.8:1) */
	{ 0x99, 0x99, 0xFF },  /* label: violet #6C71C4 → #9999FF (5.5:1) */
	{ 0xCC, 0xCC, 0x33 },  /* metadata: green #859900 → #CCCC33 (8.1:1) */
	{ 0x00, 0x66, 0x66 },  /* hover_bg: base02 #073642 → #006666 (bumped for distinction) */

	/* Selection */
	{ 0x33, 0x99, 0xCC },  /* sel_bg: blue → #3399CC */
	{ 0xFF, 0xFF, 0xCC },  /* sel_fg: base3 → #FFFFCC */
};
