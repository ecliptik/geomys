/*
 * themes/tokyo_dark.h - Tokyo Night Dark theme
 * folke/tokyonight.nvim "night" variant.
 * https://github.com/folke/tokyonight.nvim
 * Each color is the canonical Tokyo Night hex mapped to nearest 6x6x6 cube.
 */

static const ThemeColors theme_tokyo_dark = {
	"Tokyo Dark",   /* name */
	1,              /* is_color */
	1,              /* is_dark */

	/* Content area */
	{ 0x00, 0x00, 0x33 },  /* bg: #1A1B26 → #000033 */
	{ 0xCC, 0xCC, 0xFF },  /* text: fg #C0CAF5 → #CCCCFF */
	{ 0x66, 0x99, 0xFF },  /* link: blue #7AA2F7 → #6699FF */
	{ 0x00, 0xCC, 0x99 },  /* link_search: teal #1ABC9C → #00CC99 */
	{ 0xFF, 0x99, 0x66 },  /* link_external: orange #FF9E64 → #FF9966 */
	{ 0x33, 0xCC, 0xCC },  /* link_download: cyan #449DAB → #33CCCC */
	{ 0xFF, 0x66, 0x99 },  /* link_error: red #F7768E → #FF6699 */
	{ 0x66, 0x66, 0x99 },  /* label: comment #565F89 → #666699 */
	{ 0x66, 0x66, 0x99 },  /* metadata: comment → #666699 */
	{ 0x33, 0x33, 0x66 },  /* hover_bg: bg_highlight #292E42 → #333366 */

	/* Selection */
	{ 0x33, 0x66, 0x99 },  /* sel_bg: blue0 #3D59A1 → #336699 */
	{ 0xCC, 0xCC, 0xFF },  /* sel_fg: fg → #CCCCFF */

	/* Chrome */
	{ 0x00, 0x00, 0x00 },  /* chrome_bg: bg_dark #16161E → #000000 */
	{ 0x99, 0x99, 0xCC },  /* chrome_fg: fg_dark #A9B1D6 → #9999CC */
	{ 0x00, 0x00, 0x33 },  /* addr_bg: bg → #000033 */
	{ 0xCC, 0xCC, 0xFF },  /* addr_fg: fg → #CCCCFF */
};
