/*
 * themes/dracula.h - Dracula theme
 * Dark purple-accented palette from draculatheme.com.
 * Canonical Dracula RGB cube-snapped to 6x6x6 palette.
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_dracula = {
	"Dracula",      /* name */
	1,              /* is_color */
	1,              /* is_dark */

	/* Content area — Dracula palette, cube-snapped */
	{ 0x33, 0x33, 0x33 },  /* bg: #282A36 -> #333333 */
	{ 0xFF, 0xFF, 0xFF },  /* text: #F8F8F2 -> #FFFFFF */
	{ 0xCC, 0x99, 0xFF },  /* link: purple #BD93F9 -> #CC99FF (Dracula signature) */
	{ 0x99, 0xCC, 0xFF },  /* link_search: cyan #8BE9FD -> #99CCFF */
	{ 0xFF, 0x66, 0xCC },  /* link_external: pink #FF79C6 -> #FF66CC */
	{ 0x66, 0xFF, 0x99 },  /* link_download: green #50FA7B -> #66FF99 */
	{ 0xFF, 0x66, 0x66 },  /* link_error: red #FF5555 -> #FF6666 */
	{ 0x66, 0x66, 0x99 },  /* label: comment #6272A4 -> #666699 */
	{ 0x66, 0x66, 0x99 },  /* metadata: comment -> #666699 */
	{ 0x33, 0x33, 0x66 },  /* hover_bg: current_line #44475A -> #333366 */

	/* Selection */
	{ 0x66, 0x66, 0x99 },  /* sel_bg: comment #6272A4 -> #666699 */
	{ 0xFF, 0xFF, 0xFF },  /* sel_fg: foreground -> #FFFFFF */
};
