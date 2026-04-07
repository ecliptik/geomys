/*
 * themes/amber_crt.h - Amber CRT theme
 * Amber phosphor CRT aesthetic — all colors warm-shifted
 * through the amber spectrum. Cube-snapped (from Flynn).
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_amber_crt = {
	"Amber CRT",    /* name */
	1,              /* is_color */
	1,              /* is_dark */

	/* Content area — amber-tinted, cube-snapped */
	{ 0x00, 0x00, 0x00 },  /* bg: CRT black */
	{ 0xCC, 0x99, 0x00 },  /* text: amber gold #CC9900 */
	{ 0xFF, 0xCC, 0x33 },  /* link: bright amber #FFCC33 */
	{ 0x99, 0xCC, 0x33 },  /* link_search: olive-green #99CC33 */
	{ 0xFF, 0x99, 0x00 },  /* link_external: bright orange #FF9900 */
	{ 0xCC, 0xCC, 0x66 },  /* link_download: light amber #CCCC66 */
	{ 0xFF, 0x66, 0x33 },  /* link_error: red-orange #FF6633 (breaks amber monotone) */
	{ 0x66, 0x66, 0x00 },  /* label: medium amber #666600 */
	{ 0x66, 0x66, 0x00 },  /* metadata: medium amber #666600 */
	{ 0x33, 0x33, 0x00 },  /* hover_bg: dark amber #333300 */

	/* Selection */
	{ 0x66, 0x66, 0x00 },  /* sel_bg: dark amber #666600 */
	{ 0xFF, 0xCC, 0x33 },  /* sel_fg: bright amber #FFCC33 */
};
