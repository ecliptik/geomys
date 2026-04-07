/*
 * themes/system7.h - System 7 theme
 * Macintosh System 7 aesthetic with 16-color System CLUT accents.
 * Gray Platinum background, standard Mac II palette link colors.
 * Colors cube-snapped to 6x6x6 palette.
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_system7 = {
	"System 7",     /* name */
	1,              /* is_color */
	0,              /* is_dark */

	/* Content area — Mac System CLUT colors, cube-snapped */
	{ 0xCC, 0xCC, 0xCC },  /* bg: Platinum gray */
	{ 0x00, 0x00, 0x00 },  /* text: black */
	{ 0x00, 0x00, 0xCC },  /* link: Mac Blue #0000D4 -> #0000CC */
	{ 0x33, 0xCC, 0x00 },  /* link_search: Mac Green #1FB714 -> #33CC00 */
	{ 0xFF, 0x66, 0x00 },  /* link_external: Mac Orange #FF6402 -> #FF6600 */
	{ 0x00, 0x99, 0xFF },  /* link_download: Mac Cyan #02ABEA -> #0099FF */
	{ 0xCC, 0x00, 0x00 },  /* link_error: Mac Red #DD0806 -> #CC0000 */
	{ 0x66, 0x66, 0x66 },  /* label: dark gray */
	{ 0x99, 0x66, 0x33 },  /* metadata: Mac Tan #90713A -> #996633 */
	{ 0x99, 0x99, 0x99 },  /* hover_bg: medium gray */

	/* Selection */
	{ 0x33, 0x66, 0xCC },  /* sel_bg: System 7 blue highlight */
	{ 0xFF, 0xFF, 0xFF },  /* sel_fg: white */
};
