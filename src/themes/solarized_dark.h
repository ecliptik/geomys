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

	/* Content area */
	{ 0x00, 0x33, 0x33 },  /* bg: base03 #002B36 → #003333 */
	{ 0x99, 0x99, 0x99 },  /* text: base0 #839496 → #999999 */
	{ 0x33, 0x99, 0xCC },  /* link: blue #268BD2 → #3399CC */
	{ 0x33, 0x99, 0x99 },  /* link_search: cyan #2AA198 → #339999 */
	{ 0xCC, 0x33, 0x00 },  /* link_external: orange #CB4B16 → #CC3300 */
	{ 0x66, 0x99, 0x00 },  /* link_download: green #859900 → #669900 */
	{ 0xCC, 0x33, 0x33 },  /* link_error: red #DC322F → #CC3333 */
	{ 0x66, 0x66, 0xCC },  /* label: violet #6C71C4 → #6666CC */
	{ 0x99, 0x99, 0x00 },  /* metadata: green #859900 → #999900 */
	{ 0x00, 0x66, 0x66 },  /* hover_bg: base02 #073642 → #006666 (bumped for distinction) */

	/* Selection */
	{ 0x33, 0x99, 0xCC },  /* sel_bg: blue → #3399CC */
	{ 0xFF, 0xFF, 0xCC },  /* sel_fg: base3 → #FFFFCC */
};
