/*
 * themes/monokai.h - Monokai theme
 * Dark warm palette with vivid accents from Monokai Pro.
 * Colors cube-snapped to 6x6x6 palette (from Flynn).
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_monokai = {
	"Monokai",      /* name */
	1,              /* is_color */
	1,              /* is_dark */

	/* Content area — Monokai Pro, cube-snapped */
	{ 0x33, 0x33, 0x00 },  /* bg: dark olive #333300 */
	{ 0xFF, 0xFF, 0xCC },  /* text: warm white #FFFFCC */
	{ 0x66, 0x99, 0xFF },  /* link: blue #6699FF */
	{ 0x66, 0xCC, 0xCC },  /* link_search: cyan #66CCCC */
	{ 0xFF, 0xCC, 0x00 },  /* link_external: yellow #FFCC00 */
	{ 0x99, 0xCC, 0x00 },  /* link_download: green #99CC00 */
	{ 0xFF, 0x66, 0x66 },  /* link_error: bright red #FF6666 */
	{ 0x99, 0x99, 0x66 },  /* label: muted olive #999966 */
	{ 0x99, 0x99, 0x66 },  /* metadata: muted olive #999966 */
	{ 0x33, 0x33, 0x33 },  /* hover_bg: slight shift from bg */

	/* Selection */
	{ 0x66, 0x66, 0x33 },  /* sel_bg: muted olive #666633 */
	{ 0xFF, 0xFF, 0xCC },  /* sel_fg: warm white #FFFFCC */
};
