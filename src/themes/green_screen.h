/*
 * themes/green_screen.h - Green Screen (phosphor terminal) theme
 * Classic green-on-black CRT aesthetic.
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_green_screen = {
	"Green Screen",  /* name */
	1,               /* is_color */
	1,               /* is_dark */

	/* Content area */
	{ 0x00, 0x11, 0x00 },  /* bg: near-black green #001100 */
	{ 0x33, 0xFF, 0x33 },  /* text: phosphor green #33FF33 */
	{ 0x66, 0xFF, 0x66 },  /* link: bright green #66FF66 */
	{ 0x00, 0xCC, 0x66 },  /* link_search: teal green #00CC66 */
	{ 0x99, 0xFF, 0x99 },  /* link_external: pale green #99FF99 */
	{ 0x00, 0xFF, 0x99 },  /* link_download: bright teal #00FF99 */
	{ 0xFF, 0x66, 0x33 },  /* link_error: amber #FF6633 */
	{ 0x00, 0x99, 0x00 },  /* label: dim green #009900 (subdued phosphor) */
	{ 0x00, 0x99, 0x00 },  /* metadata: dim green #009900 */
	{ 0x00, 0x22, 0x00 },  /* hover_bg: slightly lighter #002200 */

	/* Selection */
	{ 0x00, 0x66, 0x00 },  /* sel_bg: dark green #006600 */
	{ 0x66, 0xFF, 0x66 },  /* sel_fg: bright green #66FF66 */

	/* Chrome */
	{ 0x00, 0x11, 0x00 },  /* chrome_bg: near-black green */
	{ 0x33, 0xFF, 0x33 },  /* chrome_fg: phosphor green */
	{ 0x00, 0x11, 0x00 },  /* addr_bg: near-black green */
	{ 0x33, 0xFF, 0x33 },  /* addr_fg: phosphor green */
};
