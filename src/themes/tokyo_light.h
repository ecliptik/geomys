/*
 * themes/tokyo_light.h - Tokyo Night Light (Day) theme
 * folke/tokyonight.nvim "day" variant.
 * https://github.com/folke/tokyonight.nvim
 * Each color is the canonical Tokyo Night Day hex mapped to nearest 6x6x6 cube.
 */

static const ThemeColors theme_tokyo_light = {
	"Tokyo Light",  /* name */
	1,              /* is_color */
	0,              /* is_dark */

	/* Content area */
	{ 0xCC, 0xCC, 0xCC },  /* bg: #E1E2E7 → #CCCCCC (neutral gray, no purple tint) */
	{ 0x33, 0x66, 0xCC },  /* text: fg #3760BF → #3366CC */
	{ 0x33, 0x66, 0xFF },  /* link: blue #2E7DE9 → #3366FF */
	{ 0x00, 0x99, 0x66 },  /* link_search: teal #118C74 → #009966 */
	{ 0x99, 0x66, 0x00 },  /* link_external: orange #B15C00 → #996600 */
	{ 0xFF, 0x33, 0x66 },  /* link_error: red #F52A65 → #FF3366 */
	{ 0x99, 0x99, 0xCC },  /* label: comment #848CB5 → #9999CC */
	{ 0x66, 0x66, 0x99 },  /* metadata: dark comment #565F89 → #666699 */
	{ 0x99, 0x99, 0xCC },  /* hover_bg: bg_highlight #C4C8DA → #9999CC */

	/* Selection */
	{ 0x33, 0x66, 0xCC },  /* sel_bg: visual #3366CC */
	{ 0xFF, 0xFF, 0xFF },  /* sel_fg: white */

	/* Chrome */
	{ 0xCC, 0xCC, 0xCC },  /* chrome_bg: bg_dark #D0D5E3 → #CCCCCC */
	{ 0x33, 0x66, 0xCC },  /* chrome_fg: fg → #3366CC */
	{ 0xFF, 0xFF, 0xFF },  /* addr_bg: white */
	{ 0x33, 0x66, 0xCC },  /* addr_fg: fg → #3366CC */
};
