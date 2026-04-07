/*
 * themes/nord.h - Nord theme
 * Arctic blue-tinted palette from nordtheme.com.
 * Canonical Nord RGB cube-snapped to 6x6x6 palette.
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_nord = {
	"Nord",         /* name */
	1,              /* is_color */
	1,              /* is_dark */

	/* Content area — Nord palette, cube-snapped */
	{ 0x33, 0x33, 0x33 },  /* bg: nord0 #2E3440 -> #333333 */
	{ 0xCC, 0xCC, 0xCC },  /* text: nord4 #D8DEE9 -> #CCCCCC */
	{ 0x99, 0x99, 0xCC },  /* link: nord9 frost #81A1C1 -> #9999CC */
	{ 0x99, 0xCC, 0xCC },  /* link_search: nord8 #88C0D0 -> #99CCCC */
	{ 0xFF, 0xCC, 0x99 },  /* link_external: nord13 #EBCB8B -> #FFCC99 */
	{ 0x99, 0xCC, 0x99 },  /* link_download: nord14 #A3BE8C -> #99CC99 */
	{ 0xCC, 0x66, 0x66 },  /* link_error: nord11 #BF616A -> #CC6666 */
	{ 0x66, 0x66, 0x66 },  /* label: muted gray */
	{ 0x66, 0x66, 0x66 },  /* metadata: muted gray */
	{ 0x33, 0x66, 0x66 },  /* hover_bg: nord3 #4C566A -> #336666 (blue-teal tint) */

	/* Selection */
	{ 0x33, 0x66, 0x99 },  /* sel_bg: frost blue */
	{ 0xCC, 0xCC, 0xCC },  /* sel_fg: nord4 -> #CCCCCC */
};
