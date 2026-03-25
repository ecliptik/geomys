/*
 * themes/solarized_light.h - Solarized Light theme
 * Ethan Schoonover's Solarized palette, light variant.
 * https://ethanschoonover.com/solarized/
 * Each color is the canonical Solarized hex mapped to nearest 6x6x6 cube.
 */

static const ThemeColors theme_solarized_light = {
	"Solarized Light",  /* name */
	1,                  /* is_color */
	0,                  /* is_dark */

	/* Content area */
	{ 0xFF, 0xFF, 0xCC },  /* bg: base3 #FDF6E3 → #FFFFCC */
	{ 0x66, 0x66, 0x99 },  /* text: base00 #657B83 → #666699 */
	{ 0x33, 0x99, 0xCC },  /* link: blue #268BD2 → #3399CC */
	{ 0x33, 0x99, 0x99 },  /* link_search: cyan #2AA198 → #339999 */
	{ 0xCC, 0x33, 0x00 },  /* link_external: orange #CB4B16 → #CC3300 */
	{ 0x66, 0x99, 0x00 },  /* link_download: green #859900 → #669900 */
	{ 0xCC, 0x33, 0x33 },  /* link_error: red #DC322F → #CC3333 */
	{ 0x66, 0x66, 0xCC },  /* label: violet #6C71C4 → #6666CC */
	{ 0x99, 0x99, 0x00 },  /* metadata: green #859900 → #999900 */
	{ 0xCC, 0xCC, 0x99 },  /* hover_bg: base2 #EEE8D5 → #CCCC99 */

	/* Selection */
	{ 0x33, 0x99, 0xCC },  /* sel_bg: blue #268BD2 → #3399CC */
	{ 0xFF, 0xFF, 0xCC },  /* sel_fg: base3 → #FFFFCC */
};
