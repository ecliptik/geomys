/*
 * themes/gruvbox.h - Gruvbox Dark theme
 * Retro earthy palette with warm accents.
 * Colors cube-snapped to 6x6x6 palette (from Flynn).
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_gruvbox = {
	"Gruvbox",      /* name */
	1,              /* is_color */
	1,              /* is_dark */

	/* Content area — Gruvbox Dark, cube-snapped */
	{ 0x33, 0x33, 0x00 },  /* bg: bg0 #333300 */
	{ 0xFF, 0xCC, 0x99 },  /* text: warm cream #FFCC99 */
	{ 0x66, 0x99, 0xCC },  /* link: blue #6699CC */
	{ 0x99, 0xCC, 0x99 },  /* link_search: aqua #99CC99 */
	{ 0xFF, 0xCC, 0x66 },  /* link_external: bright yellow #FFCC66 */
	{ 0xCC, 0xCC, 0x66 },  /* link_download: bright green #CCCC66 */
	{ 0xFF, 0x66, 0x33 },  /* link_error: bright red/orange #FF6633 */
	{ 0x99, 0x99, 0x66 },  /* label: muted #999966 */
	{ 0x99, 0x99, 0x66 },  /* metadata: muted #999966 */
	{ 0x33, 0x33, 0x33 },  /* hover_bg: slight shift from bg */

	/* Selection */
	{ 0x66, 0x66, 0x33 },  /* sel_bg: muted brown #666633 */
	{ 0xFF, 0xCC, 0x99 },  /* sel_fg: warm cream #FFCC99 */
};
