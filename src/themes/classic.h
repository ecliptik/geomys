/*
 * themes/classic.h - Classic theme
 * 1990s web browser: white background, HTML standard link colors.
 * Colors are already 256-safe (standard web palette).
 */

static const ThemeColors theme_classic = {
	"Classic",      /* name */
	1,              /* is_color */
	0,              /* is_dark */

	/* Content area — standard HTML colors */
	{ 0xFF, 0xFF, 0xFF },  /* bg: white */
	{ 0x00, 0x00, 0x00 },  /* text: black */
	{ 0x00, 0x00, 0xFF },  /* link: HTML blue #0000EE → #0000FF */
	{ 0x00, 0x66, 0x00 },  /* link_search: dark green #006400 → #006600 */
	{ 0x66, 0x00, 0x99 },  /* link_external: visited #551A8B → #660099 */
	{ 0x00, 0x99, 0x99 },  /* link_download: teal #009999 */
	{ 0xCC, 0x00, 0x00 },  /* link_error: red */
	{ 0x99, 0x99, 0x99 },  /* label: gray (subdued type labels) */
	{ 0x66, 0x66, 0x66 },  /* metadata: dark gray */
	{ 0xCC, 0xCC, 0xCC },  /* hover_bg: light gray */

	/* Selection */
	{ 0x33, 0x66, 0xCC },  /* sel_bg: selection blue */
	{ 0xFF, 0xFF, 0xFF },  /* sel_fg: white */
};
