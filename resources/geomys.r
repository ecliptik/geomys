/*
 * geomys.r - Resources for Geomys Gopher browser
 */

#include "Menus.r"
#include "Dialogs.r"
#include "Processes.r"
/* Balloon help removed — hmnu resources used too much memory
 * on System 7 for minimal user benefit */

/* SICN type definition (not in Retro68 RIncludes).
 * Each SICN is a list of 16x16 monochrome icons,
 * 32 bytes per icon (2 bytes/row * 16 rows). */
type 'SICN' {
	array {
		hex string [32];
	};
};

resource 'MBAR' (128) {
	{ 128, 129, 130, 134, 131, 132, 140, 133 }
};

resource 'MENU' (128, "Apple") {
	128, textMenuProc, allEnabled, enabled, apple,
	{
		"About Geomys\311", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (129, "File") {
	129, textMenuProc, allEnabled, enabled, "File",
	{
		"New Window", noIcon, "N", noMark, plain;
		"Open Location\311", noIcon, "L", noMark, plain;
		"Close Window", noIcon, "W", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Get Info\311", noIcon, "I", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Save As\311", noIcon, "S", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Page Setup\311", noIcon, noKey, noMark, plain;
		"Print\311", noIcon, "P", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Quit", noIcon, "Q", noMark, plain
	}
};

resource 'MENU' (130, "Edit") {
	130, textMenuProc, allEnabled, enabled, "Edit",
	{
		"Undo", noIcon, "Z", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Cut", noIcon, "X", noMark, plain;
		"Copy", noIcon, "C", noMark, plain;
		"Paste", noIcon, "V", noMark, plain;
		"Clear", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Select All", noIcon, "A", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Find\311", noIcon, "F", noMark, plain;
		"Find Again", noIcon, "G", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Show Clipboard", noIcon, "K", noMark, plain
	}
};

resource 'MENU' (134, "Go") {
	134, textMenuProc, allEnabled, enabled, "Go",
	{
		"Back", noIcon, "[", noMark, plain;
		"Forward", noIcon, "]", noMark, plain;
		"Home", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Refresh", noIcon, "R", noMark, plain;
		"Stop", noIcon, ".", noMark, plain
	}
};

resource 'MENU' (131, "Favorites") {
	131, textMenuProc, allEnabled, enabled, "Favorites",
	{
		"Manage Favorites\311", noIcon, "B", noMark, plain;
		"Add Favorite\311", noIcon, "D", noMark, plain
	}
};

resource 'MENU' (132, "Options") {
	132, textMenuProc, allEnabled, enabled, "Options",
	{
		"Font", noIcon, noKey, noMark, plain;
		"Size", noIcon, noKey, noMark, plain;
		"Page Style", noIcon, noKey, noMark, plain;
		"Theme", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Show Details", noIcon, noKey, noMark, plain;
		"Show Status Bar", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Turn Gopher+ On", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Home Page\311", noIcon, noKey, noMark, plain;
		"DNS Server\311", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (133, "Window") {
	133, textMenuProc, allEnabled, enabled, "Window",
	{
		"1 of 4 Windows", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (135, "Font") {
	135, textMenuProc, allEnabled, enabled, "Font",
	{
		"Monaco", noIcon, noKey, noMark, plain;
		"Geneva", noIcon, noKey, noMark, plain;
		"Chicago", noIcon, noKey, noMark, plain;
		"Courier", noIcon, noKey, noMark, plain;
		"New York", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (138, "Size") {
	138, textMenuProc, allEnabled, enabled, "Size",
	{
		"9", noIcon, noKey, noMark, plain;
		"10", noIcon, noKey, noMark, plain;
		"12", noIcon, noKey, noMark, plain;
		"14", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (136, "Page Style") {
	136, textMenuProc, allEnabled, enabled, "Page Style",
	{
		"Text", noIcon, noKey, noMark, plain;
		"Icons", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (137, "Theme") {
	137, textMenuProc, allEnabled, enabled, "Theme",
	{
		"Light", noIcon, noKey, noMark, plain;
		"Dark", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Solarized Light", noIcon, noKey, noMark, plain;
		"Solarized Dark", noIcon, noKey, noMark, plain;
		"TokyoNight Day", noIcon, noKey, noMark, plain;
		"TokyoNight", noIcon, noKey, noMark, plain;
		"Amber CRT", noIcon, noKey, noMark, plain;
		"System 7", noIcon, noKey, noMark, plain;
		"Dracula", noIcon, noKey, noMark, plain;
		"Nord", noIcon, noKey, noMark, plain;
		"Green Screen", noIcon, noKey, noMark, plain;
		"Classic", noIcon, noKey, noMark, plain;
		"Monokai", noIcon, noKey, noMark, plain;
		"Gruvbox", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (140, "Directory") {
	140, textMenuProc, allEnabled, enabled, "Directory",
	{
		"Geomys Home", noIcon, noKey, noMark, plain;
		"What is Gopher?", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Super-Dimensional Fortress", noIcon, noKey, noMark, plain;
		"Floodgap Systems", noIcon, noKey, noMark, plain;
		"Circumlunar Universe", noIcon, noKey, noMark, plain;
		"Bitreich", noIcon, noKey, noMark, plain;
		"HN Gopher", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Search Veronica-2", noIcon, noKey, noMark, plain
	}
};

/* About dialog — matches Flynn's layout with one extra description line */
resource 'DLOG' (130, "About Geomys") {
	{70, 100, 240, 400},
	dBoxProc,
	visible,
	noGoAway,
	0x0,
	130,
	"About Geomys",
	noAutoCenter
};

resource 'DITL' (130, "About Geomys") {
	{
		/* 1: OK button */
		{138, 115, 158, 185},
		Button { enabled, "OK" };

		/* 2: Icon */
		{10, 15, 42, 47},
		Icon { disabled, 128 };

		/* 3: App name + version */
		{10, 55, 30, 280},
		StaticText { disabled, "Geomys 1.0.1" };

		/* 4: Machine type (set at runtime) */
		{33, 55, 49, 280},
		StaticText { disabled, "" };

		/* 5: Description */
		{62, 15, 78, 290},
		StaticText { disabled, "A Gopher browser for classic Macintosh" };

		/* 6: Copyright */
		{84, 15, 100, 290},
		StaticText { disabled, "\0xA9 2026 Micheal Waltz" };

		/* 7: URL */
		{102, 15, 118, 290},
		StaticText { disabled, "https://codeberg.org/ecliptik/geomys" };

		/* 8: Default button outline (UserItem) */
		{134, 111, 162, 189},
		UserItem { disabled };
	}
};

/* Open URL dialog — movableDBoxProc for System 7 (falls back on System 6) */
/* Search query dialog — movableDBoxProc for System 7 */
resource 'DLOG' (132, "Search") {
	{90, 80, 185, 420},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	132,
	"Search",
	noAutoCenter
};

resource 'DITL' (132, "Search") {
	{
		/* 1: Search button */
		{60, 250, 80, 320},
		Button { enabled, "Search" };

		/* 2: Cancel button */
		{60, 165, 80, 235},
		Button { enabled, "Cancel" };

		/* 3: Label */
		{15, 15, 31, 320},
		StaticText { disabled, "Search" };

		/* 4: Query field */
		{35, 15, 51, 320},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{56, 246, 84, 324},
		UserItem { disabled };
	}
};

/* CSO phonebook query dialog — movableDBoxProc for System 7 */
resource 'DLOG' (142, "CSO") {
	{90, 80, 185, 420},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	142,
	"CSO Lookup",
	noAutoCenter
};

resource 'DITL' (142, "CSO") {
	{
		/* 1: Look Up button */
		{60, 245, 80, 320},
		Button { enabled, "Look Up" };

		/* 2: Cancel button */
		{60, 155, 80, 230},
		Button { enabled, "Cancel" };

		/* 3: Label (set at runtime to show CSO item name) */
		{15, 15, 31, 320},
		StaticText { disabled, "Look up in" };

		/* 4: Query field */
		{35, 15, 51, 320},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{56, 241, 84, 324},
		UserItem { disabled };
	}
};

/* Clipboard window is created programmatically in menus.c
 * per HIG p.112 (document window with scrollbar). */

/* Find in Page dialog — movableDBoxProc for System 7 */
resource 'DLOG' (137, "Find") {
	{90, 80, 185, 420},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	137,
	"Find",
	noAutoCenter
};

resource 'DITL' (137, "Find") {
	{
		/* 1: Find button */
		{60, 250, 80, 320},
		Button { enabled, "Find" };

		/* 2: Cancel button */
		{60, 165, 80, 235},
		Button { enabled, "Cancel" };

		/* 3: Label */
		{15, 15, 31, 60},
		StaticText { disabled, "Find" };

		/* 4: Query field */
		{35, 15, 51, 320},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{56, 246, 84, 324},
		UserItem { disabled };
	}
};

/* HTML URL dialog — shows external URL for type h items */
resource 'DLOG' (138, "HTML URL") {
	{80, 50, 195, 450},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	138,
	"External Link",
	noAutoCenter
};

resource 'DITL' (138, "HTML URL") {
	{
		/* 1: OK button */
		{80, 310, 100, 380},
		Button { enabled, "OK" };

		/* 2: Cancel button */
		{80, 220, 100, 290},
		Button { enabled, "Cancel" };

		/* 3: Description text */
		{10, 15, 42, 385},
		StaticText { disabled, "This link points to a web page outside of Gopher" };

		/* 4: URL field (editable for Cmd-C copy) */
		{50, 15, 66, 385},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{76, 306, 104, 384},
		UserItem { disabled };
	}
};

/* Home Page dialog — movableDBoxProc for System 7 */
resource 'DLOG' (133, "Home Page") {
	{80, 80, 220, 420},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	133,
	"Home Page",
	noAutoCenter
};

resource 'DITL' (133, "Home Page") {
	{
		/* 1: OK button */
		{105, 250, 125, 320},
		Button { enabled, "OK" };

		/* 2: Cancel button */
		{105, 160, 125, 230},
		Button { enabled, "Cancel" };

		/* 3: URL label */
		{15, 15, 31, 100},
		StaticText { disabled, "Home Page" };

		/* 4: URL field */
		{15, 105, 31, 325},
		EditText { enabled, "" };

		/* 5: Use Blank checkbox */
		{70, 15, 86, 200},
		CheckBox { enabled, "Use Blank Page" };

		/* 6: Default button outline (UserItem) */
		{101, 246, 129, 324},
		UserItem { disabled };

		/* 7: Use Current Page button */
		{43, 105, 63, 245},
		Button { enabled, "Use Current Page" };
	}
};

/* Favorites manager dialog — movableDBoxProc for System 7 */
resource 'DLOG' (134, "Favorites") {
	{40, 60, 300, 430},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	134,
	"Favorites",
	noAutoCenter
};

resource 'DITL' (134, "Favorites") {
	{
		/* 1: Done button */
		{228, 280, 248, 350},
		Button { enabled, "Done" };

		/* 2: Add button */
		{15, 280, 35, 350},
		Button { enabled, "Add" };

		/* 3: Edit button */
		{45, 280, 65, 350},
		Button { enabled, "Edit" };

		/* 4: Label */
		{5, 15, 21, 120},
		StaticText { disabled, "Favorites" };

		/* 5: List area (UserItem — enabled for click detection) */
		{25, 15, 218, 265},
		UserItem { enabled };

		/* 6: Default button outline (UserItem) */
		{224, 276, 252, 354},
		UserItem { disabled };

		/* 7: Remove button */
		{75, 280, 95, 350},
		Button { enabled, "Remove" };

		/* 8: Move Up button */
		{115, 280, 135, 350},
		Button { enabled, "Move Up" };

		/* 9: Move Down button */
		{145, 280, 165, 350},
		Button { enabled, "Move Dn" };

		/* 10: Go To button */
		{195, 280, 215, 350},
		Button { enabled, "Go To" };
	}
};

/* Edit/Add Favorite dialog — movableDBoxProc for System 7 */
resource 'DLOG' (135, "Edit Favorite") {
	{90, 90, 210, 420},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	135,
	"Edit Favorite",
	noAutoCenter
};

resource 'DITL' (135, "Edit Favorite") {
	{
		/* 1: OK button */
		{85, 245, 105, 315},
		Button { enabled, "OK" };

		/* 2: Cancel button */
		{85, 155, 105, 225},
		Button { enabled, "Cancel" };

		/* 3: Name label */
		{15, 15, 31, 70},
		StaticText { disabled, "Name" };

		/* 4: Name field */
		{15, 75, 31, 315},
		EditText { enabled, "" };

		/* 5: URL label */
		{45, 15, 61, 70},
		StaticText { disabled, "URL" };

		/* 6: URL field */
		{45, 75, 61, 315},
		EditText { enabled, "" };

		/* 7: Default button outline (UserItem) */
		{81, 241, 109, 319},
		UserItem { disabled };
	}
};

/* Remove confirmation alert — Cancel is default (item 1) per HIG */
resource 'ALRT' (129) {
	{60, 80, 180, 420},
	129,
	{
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1
	},
	noAutoCenter
};

resource 'DITL' (129, "Remove Confirm") {
	{
		/* 1: Cancel button (default — safe action) */
		{85, 250, 105, 320},
		Button { enabled, "Cancel" };

		/* 2: Remove button (destructive) */
		{85, 140, 105, 210},
		Button { enabled, "Remove" };

		/* 3: Text */
		{15, 75, 70, 325},
		StaticText { disabled, "^0" };

		/* 4: Default button outline (UserItem) */
		{81, 246, 109, 324},
		UserItem { disabled };
	}
};

/* Generic alert for ParamText messages */
resource 'ALRT' (128) {
	{60, 80, 180, 420},
	128,
	{
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1
	},
	noAutoCenter
};

resource 'DITL' (128, "Alert") {
	{
		/* 1: OK button */
		{85, 250, 105, 320},
		Button { enabled, "OK" };

		/* 2: Cancel button */
		{85, 160, 105, 230},
		Button { enabled, "Cancel" };

		/* 3: Text */
		{15, 75, 70, 325},
		StaticText { disabled, "^0" };

		/* 4: Default button outline (UserItem) */
		{81, 246, 109, 324},
		UserItem { disabled };
	}
};

/* Download progress dialog — movable modal with Stop button */
resource 'DLOG' (139, "Download Progress") {
	{80, 90, 170, 410},
	movableDBoxProc,
	visible,
	noGoAway,
	0,
	139,
	"Downloading",
	alertPositionMainScreen
};

resource 'DITL' (139, "Download Progress") {
	{
		/* 1: Stop button (centered at bottom) */
		{55, 120, 75, 200},
		Button { enabled, "Stop" };

		/* 2: Filename text */
		{10, 15, 26, 305},
		StaticText { disabled, "Downloading ^0" };

		/* 3: Byte count text */
		{30, 15, 46, 305},
		StaticText { disabled, "^1 bytes received" };
	}
};

/* Telnet connection dialog — shows host, port, optional login */
resource 'DLOG' (140, "Telnet") {
	{70, 60, 260, 420},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	140,
	"Telnet Connection",
	noAutoCenter
};

resource 'DITL' (140, "Telnet") {
	{
		/* 1: Done button (default, rightmost) */
		{155, 270, 175, 340},
		Button { enabled, "Done" };

		/* 2: Cancel button (hidden, for Escape/Cmd-.) */
		{155, 270, 175, 340},
		Button { enabled, "" };

		/* 3: Copy Host button */
		{155, 170, 175, 255},
		Button { enabled, "Copy Host" };

		/* 4: Display name (title of the item) */
		{10, 15, 26, 345},
		StaticText { disabled, "^0" };

		/* 5: Host label */
		{38, 15, 54, 55},
		StaticText { disabled, "Host" };

		/* 6: Host value (editable for Cmd-C) */
		{38, 60, 54, 345},
		EditText { enabled, "" };

		/* 7: Port label */
		{62, 15, 78, 55},
		StaticText { disabled, "Port" };

		/* 8: Port value */
		{62, 60, 78, 120},
		StaticText { disabled, "" };

		/* 9: Login label */
		{86, 15, 102, 55},
		StaticText { disabled, "Login" };

		/* 10: Login value */
		{86, 60, 102, 345},
		StaticText { disabled, "" };

		/* 11: Instructions text */
		{112, 15, 144, 345},
		StaticText { disabled, "To connect, copy the host and port, then open your telnet application." };

		/* 12: Default button outline (UserItem) */
		{151, 266, 179, 344},
		UserItem { disabled };

		/* 13: TN3270 note (hidden unless type T) */
		{112, 15, 144, 345},
		StaticText { disabled, "" };
	}
};

resource 'DLOG' (141, "DNS Server") {
	{80, 100, 175, 400},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	141,
	"DNS Server",
	noAutoCenter
};

resource 'DITL' (141, "DNS Server") {
	{
		/* 1: OK button */
		{60, 210, 80, 280},
		Button { enabled, "OK" };

		/* 2: Cancel button */
		{60, 120, 80, 190},
		Button { enabled, "Cancel" };

		/* 3: DNS Server label */
		{15, 15, 31, 110},
		StaticText { disabled, "DNS Server" };

		/* 4: DNS Server IP field */
		{15, 115, 31, 275},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{56, 206, 84, 284},
		UserItem { disabled };
	}
};

/* Gopher+ Get Info dialog — movableDBoxProc */
resource 'DLOG' (143, "Get Info") {
	{70, 80, 325, 430},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	143,
	"Get Info",
	noAutoCenter
};

resource 'DITL' (143, "Get Info") {
	{
		/* 1: Done button (default) */
		{222, 267, 242, 337},
		Button { enabled, "Done" };

		/* 2: Item display name (set at runtime) */
		{10, 13, 30, 337},
		StaticText { disabled, "" };

		/* 3: Type (set at runtime) */
		{35, 13, 51, 337},
		StaticText { disabled, "" };

		/* 4: Server (set at runtime) */
		{55, 13, 71, 337},
		StaticText { disabled, "" };

		/* 5: Selector (set at runtime) */
		{75, 13, 91, 337},
		StaticText { disabled, "" };

		/* 6: Admin (set at runtime, from +ADMIN) */
		{100, 13, 116, 337},
		StaticText { disabled, "" };

		/* 7: Modified date (set at runtime, from +ADMIN) */
		{120, 13, 136, 337},
		StaticText { disabled, "" };

		/* 8: Available views (set at runtime, from +VIEWS) */
		{140, 13, 172, 337},
		StaticText { disabled, "" };

		/* 9: Abstract (set at runtime, from +ABSTRACT) */
		{176, 13, 216, 337},
		StaticText { disabled, "" };

		/* 10: Choose View button (dimmed when <= 1 view) */
		{222, 13, 242, 113},
		Button { enabled, "Choose View\311" };

		/* 11: Fill Form button (dimmed when no +ASK) */
		{222, 123, 242, 213},
		Button { enabled, "Fill Form\311" };

		/* 12: Default button outline (UserItem) */
		{218, 263, 246, 341},
		UserItem { disabled };
	}
};

/* Gopher+ View Selection dialog — movableDBoxProc */
resource 'DLOG' (144, "Choose View") {
	{80, 100, 290, 390},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	144,
	"Choose View",
	noAutoCenter
};

resource 'DITL' (144, "Choose View") {
	{
		/* 1: Open button (default) */
		{178, 200, 198, 270},
		Button { enabled, "Open" };

		/* 2: Cancel button */
		{178, 115, 198, 185},
		Button { enabled, "Cancel" };

		/* 3-10: Radio buttons for views (text set at runtime) */
		{10, 20, 26, 270},
		RadioButton { enabled, "" };

		{30, 20, 46, 270},
		RadioButton { enabled, "" };

		{50, 20, 66, 270},
		RadioButton { enabled, "" };

		{70, 20, 86, 270},
		RadioButton { enabled, "" };

		{90, 20, 106, 270},
		RadioButton { enabled, "" };

		{110, 20, 126, 270},
		RadioButton { enabled, "" };

		{130, 20, 146, 270},
		RadioButton { enabled, "" };

		{150, 20, 166, 270},
		RadioButton { enabled, "" };

		/* 11: Default button outline (UserItem) */
		{174, 196, 202, 274},
		UserItem { disabled };
	}
};

/* Open Location dialog — movableDBoxProc for System 7 */
resource 'DLOG' (145, "Open Location") {
	{90, 60, 185, 440},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	145,
	"Open Location",
	noAutoCenter
};

resource 'DITL' (145, "Open Location") {
	{
		/* 1: Open button (default) */
		{60, 290, 80, 360},
		Button { enabled, "Open" };

		/* 2: Cancel button */
		{60, 200, 80, 270},
		Button { enabled, "Cancel" };

		/* 3: Label */
		{15, 15, 31, 360},
		StaticText { disabled, "Enter a Gopher URL" };

		/* 4: URL field */
		{35, 15, 51, 360},
		EditText { enabled, "gopher://" };

		/* 5: Default button outline (UserItem) */
		{56, 286, 84, 364},
		UserItem { disabled };
	}
};

/* Hand cursor for hovering over navigable items */
/* Classic pointing hand — open palm with index finger up */
data 'CURS' (129) {
	/* Cursor bitmap (16x16) */
	$"1800 2400 2400 2400"
	$"2400 27C0 2DB0 3938"
	$"3828 2028 2028 1028"
	$"1010 0810 0410 0410"
	/* Mask bitmap (16x16) */
	$"1800 3C00 3C00 3C00"
	$"3C00 3FC0 3FF0 3FF8"
	$"3FF8 3FF8 3FF8 1FF8"
	$"1FF0 0FF0 07F0 07F0"
	/* Hotspot (tip of finger) */
	$"0000 0004"
};

/* Application icon - 32x32 bitmap for About dialog */
/* Compact Mac with Gopher directory listing on screen (Flynn style) */
data 'ICON' (128) {
	$"00000000 03FFFF00"  /* row 0-1: top bezel */
	$"07FFFF80 0FFFFFC0"  /* row 2-3: bezel */
	$"0E0001C0 0C0000C0"  /* row 4-5: screen top */
	$"0CBFC0C0 0C0000C0"  /* row 6-7: bullet+line, blank */
	$"0CB7E0C0 0C0000C0"  /* row 8-9: bullet+line, blank */
	$"0CBF00C0 0C0000C0"  /* row 10-11: bullet+line (short), blank */
	$"0CBFE0C0 0C0000C0"  /* row 12-13: bullet+line, blank */
	$"0CB780C0 0E0001C0"  /* row 14-15: bullet+line (short), screen bottom */
	$"0FFFFFC0 0C0000C0"  /* row 16-17: bezel line, base */
	$"0C0000C0 0C0000C0"  /* row 18-19: base */
	$"0C007EC0 0C0000C0"  /* row 20-21: floppy slot */
	$"0C0000C0 0FFFFFC0"  /* row 22-23: base bottom */
	$"07FFFF80 03FFFF00"  /* row 24-25: feet */
	$"00000000 00000000"  /* row 26-27 */
	$"00000000 00000000"  /* row 28-29 */
	$"00000000 00000000"  /* row 30-31 */
};

/* Application icon - 32x32 1-bit + mask */
data 'ICN#' (128) {
	/* Icon bitmap (same as ICON) */
	$"00000000 03FFFF00"
	$"07FFFF80 0FFFFFC0"
	$"0E0001C0 0C0000C0"
	$"0CBFC0C0 0C0000C0"
	$"0CB7E0C0 0C0000C0"
	$"0CBF00C0 0C0000C0"
	$"0CBFE0C0 0C0000C0"
	$"0CB780C0 0E0001C0"
	$"0FFFFFC0 0C0000C0"
	$"0C0000C0 0C0000C0"
	$"0C007EC0 0C0000C0"
	$"0C0000C0 0FFFFFC0"
	$"07FFFF80 03FFFF00"
	$"00000000 00000000"
	$"00000000 00000000"
	$"00000000 00000000"
	/* Mask bitmap */
	$"00000000 03FFFF00"
	$"07FFFF80 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"0FFFFFC0 0FFFFFC0"
	$"07FFFF80 03FFFF00"
	$"00000000 00000000"
	$"00000000 00000000"
	$"00000000 00000000"
};

/* Application icon - 16x16 1-bit + mask */
/* Compact Mac mini with directory listing on screen */
data 'ics#' (128) {
	/* Icon bitmap */
	$"1FF0"  /* row 0:  ...#########.... — top bezel */
	$"3FF8"  /* row 1:  ..###########... — bezel */
	$"6008"  /* row 2:  .##.........#... — screen top */
	$"6F88"  /* row 3:  .##.#####...#... — bullet+line */
	$"6D88"  /* row 4:  .##.##.##...#... — bullet+line */
	$"6F08"  /* row 5:  .##.####....#... — bullet+line short */
	$"6DE8"  /* row 6:  .##.##.####.#... — bullet+line */
	$"6008"  /* row 7:  .##.........#... — screen bottom */
	$"7FF8"  /* row 8:  .###########.... — bezel line */
	$"6008"  /* row 9:  .##.........#... — base */
	$"63C8"  /* row 10: .##...####..#... — floppy */
	$"6008"  /* row 11: .##.........#... — base */
	$"7FF8"  /* row 12: .###########.... — base bottom */
	$"3FF0"  /* row 13: ..#########..... — feet */
	$"0000"  /* row 14 */
	$"0000"  /* row 15 */
	/* Mask bitmap */
	$"1FF0"
	$"3FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"7FF8"
	$"3FF0"
	$"0000"
	$"0000"
};

/* 32x32 4-bit color icon */
data 'icl4' (128) {
	$"00000000000000000000000000000000"
	$"000000FFFFFFFFFFFFFFFFFF00000000"
	$"00000FCCCCCCCCCCCCCCCCCCF0000000"
	$"0000FCCCCCCCCCCCCCCCCCCCCF000000"
	$"0000FEE0000000000000000EEF000000"
	$"0000FE000000000000000000EF000000"
	$"0000FE00F0FFFFFFFF000000EF000000"
	$"0000FE000000000000000000EF000000"
	$"0000FE00F0FF0FFFFFF00000EF000000"
	$"0000FE000000000000000000EF000000"
	$"0000FE00F0FFFFFF00000000EF000000"
	$"0000FE000000000000000000EF000000"
	$"0000FE00F0FFFFFFFFF00000EF000000"
	$"0000FE000000000000000000EF000000"
	$"0000FE00F0FF0FFFF0000000EF000000"
	$"0000FEE0000000000000000EEF000000"
	$"0000FDDDDDDDDDDDDDDDDDDDDF000000"
	$"0000FCCCCCCCCCCCCCCCCCCCCF000000"
	$"0000FCCCCCCCCCCCCCCCCCCCCF000000"
	$"0000FCCCCCCCCCCCCCCCCCCCCF000000"
	$"0000FCCCCCCCCCCCCEEEEEECCF000000"
	$"0000FCCCCCCCCCCCCCCCCCCCCF000000"
	$"0000FCCCCCCCCCCCCCCCCCCCCF000000"
	$"0000FFFFFFFFFFFFFFFFFFFFFF000000"
	$"00000FCCCCCCCCCCCCCCCCCCF0000000"
	$"000000FFFFFFFFFFFFFFFFFF00000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
};

/* 32x32 8-bit color icon */
data 'icl8' (128) {
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"000000000000FFFFFFFFFFFFFFFFFFFF"
	$"FFFFFFFFFFFFFFFF0000000000000000"
	$"0000000000FF2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2BFF00000000000000"
	$"00000000FF2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2B2BFF000000000000"
	$"00000000FF8181000000000000000000"
	$"000000000000008181FF000000000000"
	$"00000000FF8100000000000000000000"
	$"000000000000000081FF000000000000"
	$"00000000FF810000FF00FFFFFFFFFFFF"
	$"FFFF00000000000081FF000000000000"
	$"00000000FF8100000000000000000000"
	$"000000000000000081FF000000000000"
	$"00000000FF810000FF00FFFF00FFFFFF"
	$"FFFFFF000000000081FF000000000000"
	$"00000000FF8100000000000000000000"
	$"000000000000000081FF000000000000"
	$"00000000FF810000FF00FFFFFFFFFFFF"
	$"000000000000000081FF000000000000"
	$"00000000FF8100000000000000000000"
	$"000000000000000081FF000000000000"
	$"00000000FF810000FF00FFFFFFFFFFFF"
	$"FFFFFF000000000081FF000000000000"
	$"00000000FF8100000000000000000000"
	$"000000000000000081FF000000000000"
	$"00000000FF810000FF00FFFF00FFFFFF"
	$"FF0000000000000081FF000000000000"
	$"00000000FF8181000000000000000000"
	$"000000000000008181FF000000000000"
	$"00000000FF5656565656565656565656"
	$"565656565656565656FF000000000000"
	$"00000000FF2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2B2BFF000000000000"
	$"00000000FF2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2B2BFF000000000000"
	$"00000000FF2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2B2BFF000000000000"
	$"00000000FF2B2B2B2B2B2B2B2B2B2B2B"
	$"2B8181818181812B2BFF000000000000"
	$"00000000FF2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2B2BFF000000000000"
	$"00000000FF2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2B2BFF000000000000"
	$"00000000FFFFFFFFFFFFFFFFFFFFFFFF"
	$"FFFFFFFFFFFFFFFFFFFF000000000000"
	$"0000000000FF2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2B2BFF00000000000000"
	$"000000000000FFFFFFFFFFFFFFFFFFFF"
	$"FFFFFFFFFFFFFFFF0000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
};

/* 16x16 4-bit color icon */
data 'ics4' (128) {
	$"000FFFFFFFFF0000"
	$"00FCCCCCCCCCF000"
	$"0FE000000000F000"
	$"0FE0FFFFF000F000"
	$"0FE0FF0FF000F000"
	$"0FE0FFFF0000F000"
	$"0FE0FF0FFFF0F000"
	$"0FE000000000F000"
	$"0FDDDDDDDDDDF000"
	$"0FCCCCCCCCCCF000"
	$"0FCCCCEEEECCF000"
	$"0FCCCCCCCCCCF000"
	$"0FFFFFFFFFFFF000"
	$"00FCCCCCCCCF0000"
	$"0000000000000000"
	$"0000000000000000"
};

/* 16x16 8-bit color icon */
data 'ics8' (128) {
	$"000000FFFFFFFFFFFFFFFFFF00000000"
	$"0000FF2B2B2B2B2B2B2B2B2BFF000000"
	$"00FF81000000000000000000FF000000"
	$"00FF8100FFFFFFFFFF000000FF000000"
	$"00FF8100FFFF00FFFF000000FF000000"
	$"00FF8100FFFFFFFF00000000FF000000"
	$"00FF8100FFFF00FFFFFFFF00FF000000"
	$"00FF81000000000000000000FF000000"
	$"00FF56565656565656565656FF000000"
	$"00FF2B2B2B2B2B2B2B2B2B2BFF000000"
	$"00FF2B2B2B2B818181812B2BFF000000"
	$"00FF2B2B2B2B2B2B2B2B2B2BFF000000"
	$"00FFFFFFFFFFFFFFFFFFFFFFFF000000"
	$"0000FF2B2B2B2B2B2B2B2BFF00000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
};

/* Preferences document icon - 32x32 1-bit + mask */
/* Directory listing on paper (hierarchy document) */
data 'ICN#' (129) {
	/* Icon bitmap */
	$"1FFFF800" $"10000C00"
	$"10000A00" $"10000900"
	$"10000F00" $"10000100"
	$"10000100" $"13F80100"
	$"10000100" $"10000100"
	$"11BF0100" $"10000100"
	$"11BFC100" $"10000100"
	$"11BC0100" $"10000100"
	$"11BFE100" $"10000100"
	$"11BF8100" $"10000100"
	$"11BE0100" $"10000100"
	$"11BFF100" $"10000100"
	$"11BF0100" $"10000100"
	$"11BFC100" $"10000100"
	$"10000100" $"1FFFF100"
	$"00000000" $"00000000"
	/* Mask bitmap */
	$"1FFFF800" $"1FFFFC00"
	$"1FFFFE00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"1FFFFF00" $"1FFFFF00"
	$"00000000" $"00000000"
};

/* Preferences document icon - 16x16 1-bit + mask */
data 'ics#' (129) {
	/* Icon bitmap */
	$"7FE0"  /* top */
	$"4030"  /* dog-ear */
	$"4028"
	$"403C"  /* fold */
	$"4004"
	$"5E04"  /* title line */
	$"4004"
	$"4D84"  /* items */
	$"4DE4"
	$"4D04"
	$"4DF4"
	$"4D84"
	$"4DC4"
	$"4004"
	$"7FFC"  /* bottom */
	$"0000"
	/* Mask bitmap */
	$"7FE0"
	$"7FF0"
	$"7FF8"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"7FFC"
	$"0000"
};

/* Preferences document icon - 32x32 4-bit color */
data 'icl4' (129) {
	$"000FFFFFFFFFFFFFFFFFF00000000000"
	$"000FCCCCCCCCCCCCCCCCFF0000000000"
	$"000FCCCCCCCCCCCCCCCCFEF000000000"
	$"000FCCCCCCCCCCCCCCCCFEEF00000000"
	$"000FCCCCCCCCCCCCCCCCFFFF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCFFFFFFFCCCCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFFCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFFFFCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFCCCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFFFFFCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFFFCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFCCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFFFFFFCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFFCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCFFCFFFFFFFFCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FCCCCCCCCCCCCCCCCCCCF00000000"
	$"000FFFFFFFFFFFFFFFFFFFFF00000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
};

/* Preferences document icon - 32x32 8-bit color */
data 'icl8' (129) {
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFF"
	$"FFFFFFFFFF0000000000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2BFFFF00000000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2BFF81FF000000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2BFF8181FF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2BFFFFFFFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2BFFFFFFFFFFFFFF2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFFFF"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFFFF"
	$"FFFF2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFF2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFFFF"
	$"FFFFFF2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFFFF"
	$"FF2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFF2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFFFF"
	$"FFFFFFFF2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFFFF"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2BFFFF2BFFFFFFFFFFFF"
	$"FFFF2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FF2B2B2B2B2B2B2B2B2B2B2B2B"
	$"2B2B2B2B2B2B2BFF0000000000000000"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFF"
	$"FFFFFFFFFFFFFFFF0000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
	$"00000000000000000000000000000000"
};

/* Preferences document icon - 16x16 4-bit color */
data 'ics4' (129) {
	$"0FFFFFFFFFF00000"
	$"0FCCCCCCCCFF0000"
	$"0FCCCCCCCCFEF000"
	$"0FCCCCCCCCFFFF00"
	$"0FCCCCCCCCCCCF00"
	$"0FCFFFFCCCCCCF00"
	$"0FCCCCCCCCCCCF00"
	$"0FCCFFCFFCCCCF00"
	$"0FCCFFCFFFFCCF00"
	$"0FCCFFCFCCCCCF00"
	$"0FCCFFCFFFFFCF00"
	$"0FCCFFCFFCCCCF00"
	$"0FCCFFCFFFCCCF00"
	$"0FCCCCCCCCCCCF00"
	$"0FFFFFFFFFFFFF00"
	$"0000000000000000"
};

/* Preferences document icon - 16x16 8-bit color */
data 'ics8' (129) {
	$"00FFFFFFFFFFFFFFFFFFFF0000000000"
	$"00FF2B2B2B2B2B2B2B2BFFFF00000000"
	$"00FF2B2B2B2B2B2B2B2BFF81FF000000"
	$"00FF2B2B2B2B2B2B2B2BFFFFFFFF0000"
	$"00FF2B2B2B2B2B2B2B2B2B2B2BFF0000"
	$"00FF2BFFFFFFFF2B2B2B2B2B2BFF0000"
	$"00FF2B2B2B2B2B2B2B2B2B2B2BFF0000"
	$"00FF2B2BFFFF2BFFFF2B2B2B2BFF0000"
	$"00FF2B2BFFFF2BFFFFFFFF2B2BFF0000"
	$"00FF2B2BFFFF2BFF2B2B2B2B2BFF0000"
	$"00FF2B2BFFFF2BFFFFFFFFFF2BFF0000"
	$"00FF2B2BFFFF2BFFFF2B2B2B2BFF0000"
	$"00FF2B2BFFFF2BFFFFFF2B2B2BFF0000"
	$"00FF2B2B2B2B2B2B2B2B2B2B2BFF0000"
	$"00FFFFFFFFFFFFFFFFFFFFFFFFFFFF0000"
	$"00000000000000000000000000000000"
};

/* Preferences document ICON for About dialog */
data 'ICON' (129) {
	$"1FFFF800"
	$"10000C00"
	$"10000A00"
	$"10000900"
	$"10000F00"
	$"10000100"
	$"10000100"
	$"13F80100"
	$"10000100"
	$"10000100"
	$"11BF0100"
	$"10000100"
	$"11BFC100"
	$"10000100"
	$"11BC0100"
	$"10000100"
	$"11BFE100"
	$"10000100"
	$"11BF8100"
	$"10000100"
	$"11BE0100"
	$"10000100"
	$"11BFF100"
	$"10000100"
	$"11BF0100"
	$"10000100"
	$"11BFC100"
	$"10000100"
	$"10000100"
	$"1FFFF100"
	$"00000000"
	$"00000000"
};

/* File reference - APPL type, local icon 0 */
data 'FREF' (128) {
	$"4150 504C 0000 00"                                  /* APPL... */
};

/* File reference - pref type, local icon 1 */
data 'FREF' (129) {
	$"7072 6566 0001 00"                                  /* pref... */
};

/* Bundle - associates creator 'GEOM' with icons and file refs */
data 'BNDL' (128) {
	$"4745 4F4D"                                          /* GEOM */
	$"0000"                                               /* owner ID */
	$"0001"                                               /* 2 types */
	$"4652 4546"                                          /* FREF */
	$"0001"                                               /* 2 entries */
	$"0000 0080"                                          /* local 0 -> res 128 (APPL) */
	$"0001 0081"                                          /* local 1 -> res 129 (pref) */
	$"4943 4E23"                                          /* ICN# */
	$"0001"                                               /* 2 entries */
	$"0000 0080"                                          /* local 0 -> res 128 (app icon) */
	$"0001 0081"                                          /* local 1 -> res 129 (prefs icon) */
};

/* Application signature string */
data 'GEOM' (0, "Owner resource") {
	$"19"                                                 /* Pascal string length */
	"Geomys - Gopher Browser"
};

/* AppleScript terminology — defines scriptable vocabulary.
 * Suite 'GEOM' with events: navigate (GURL), get URL (gURL). */
data 'aete' (0, "Geomys Scripting") {
	/* Header: version 1.0, English, Roman, 1 suite */
	$"0100 0000 0000 0001"

	/* Suite name: "Geomys Suite" (padded) */
	$"0C" "Geomys Suite" $"00"
	/* Suite description */
	$"13" "Commands for Geomys"
	/* Suite ID, level, version */
	$"4745 4F4D"                   /* GEOM */
	$"0001 0001"                   /* level 1, version 1 */

	/* 2 events */
	$"0002"

	/* Event 1: navigate — navigate to a gopher:// URL */
	$"08" "navigate" $"00"         /* name (padded) */
	$"11" "Navigate to a URL"      /* description */
	$"4745 4F4D"                   /* class: GEOM */
	$"4755 524C"                   /* ID: GURL */
	/* Reply: none */
	$"6E75 6C6C"                   /* type: null */
	$"00" $"00"                    /* description: empty (padded) */
	$"0000"                        /* flags */
	/* Direct parameter: URL text */
	$"5445 5854"                   /* type: TEXT */
	$"07" "The URL"                /* description */
	$"8000"                        /* flags: required */
	/* No other parameters */
	$"0000"

	/* Event 2: get URL — return current page URL */
	$"07" "get URL"                /* name */
	$"14" "Get current page URL" $"00"  /* description (padded) */
	$"4745 4F4D"                   /* class: GEOM */
	$"6755 524C"                   /* ID: gURL */
	/* Reply: URL text */
	$"5445 5854"                   /* type: TEXT */
	$"0F" "The current URL"        /* description */
	$"8000"                        /* flags: required */
	/* Direct parameter: none */
	$"6E75 6C6C"                   /* type: null */
	$"00" $"00"                    /* description: empty (padded) */
	$"0000"                        /* flags */
	/* No other parameters */
	$"0000"

	/* 0 classes, 0 comparison ops, 0 enumerations */
	$"0000 0000 0000"
};

resource 'SIZE' (-1) {
	reserved,
	acceptSuspendResumeEvents,
	reserved,
	canBackground,
	doesActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	isHighLevelEventAware,
	onlyLocalHLEvents,
	isStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	2560 * 1024,
	1536 * 1024
};

/* ===== SICN resources for Gopher type icons (16x16) ===== */
/*
 * Small Icon (SICN) resources for Gopher item types.
 * Used when font row height >= 16 (e.g. Monaco 12, Chicago 12).
 * Each icon is 16x16 pixels, 2 bytes/row, 32 bytes total.
 * IDs 256-266 correspond to gopher item type icons.
 */

/* 256: Folder - classic Mac folder with tab */
resource 'SICN' (256, "Folder") {
	{
		$"0000 7E00 7FFC 4004 4004 4004 4004 4004"
		$"4004 4004 4004 7FFC 0000 0000 0000 0000"
	}
};

/* 257: Document - page with dog-ear corner */
resource 'SICN' (257, "Document") {
	{
		$"0000 3FC0 20F0 2010 2010 2010 2010 2010"
		$"2010 2010 2010 2010 3FF0 0000 0000 0000"
	}
};

/* 258: Search - folder with question mark */
resource 'SICN' (258, "Search") {
	{
		$"0000 7E00 7FFC 4004 4784 40C4 4184 4304"
		$"4004 4304 4004 7FFC 0000 0000 0000 0000"
	}
};

/* 259: Error - warning triangle with exclamation */
resource 'SICN' (259, "Error") {
	{
		$"0000 0100 0280 0280 0540 0540 0920 0920"
		$"1010 1110 2008 3FF8 0000 0000 0000 0000"
	}
};

/* 260: Binary - package box with divider */
resource 'SICN' (260, "Binary") {
	{
		$"0000 7FF8 4008 4008 4008 4008 4008 7FF8"
		$"4008 4008 4008 4008 4008 7FF8 0000 0000"
	}
};

/* 261: Terminal - monitor with cursor and base */
resource 'SICN' (261, "Terminal") {
	{
		$"0000 7FF8 4008 5808 4008 4008 4008 4008"
		$"4008 7FF8 0300 0FC0 0000 0000 0000 0000"
	}
};

/* 262: Image - picture frame with sun and mountain */
resource 'SICN' (262, "Image") {
	{
		$"0000 7FF8 4008 5808 4008 4008 4008 4048"
		$"40A8 4118 4008 7FF8 0000 0000 0000 0000"
	}
};

/* 263: Globe - circle with meridian and equator */
resource 'SICN' (263, "Globe") {
	{
		$"0000 0FC0 3330 4308 4308 4308 7FFC 4308"
		$"4308 4308 3330 0FC0 0000 0000 0000 0000"
	}
};

/* 264: Speaker - cone with sound wave arcs */
resource 'SICN' (264, "Speaker") {
	{
		$"0000 0000 0400 0600 0700 3F10 3F28 3F44"
		$"3F28 3F10 0700 0600 0400 0000 0000 0000"
	}
};

/* 265: Phonebook - book with ruled entries */
resource 'SICN' (265, "Phonebook") {
	{
		$"0000 7FF0 4010 5FD0 4010 5FD0 4010 5FD0"
		$"4010 5FD0 4010 5FD0 4010 7FF0 0000 0000"
	}
};

/* 266: Unknown - box with question mark */
resource 'SICN' (266, "Unknown") {
	{
		$"0000 7FF8 4008 4788 40C8 4188 4308 4308"
		$"4008 4308 4308 4008 7FF8 0000 0000 0000"
	}
};

/* 289: Menu Folder - duplicate of SICN 256 for menu icon use.
 * SetItemIcon(iconNum=0) means "no icon", so SICN 256
 * (iconNum 0) can't be used in menus. SICN 289 (iconNum 33)
 * provides the folder icon at a menu-safe resource ID. */
resource 'SICN' (289, "Menu Folder") {
	{
		$"0000 7E00 7FFC 4004 4004 4004 4004 4004"
		$"4004 4004 4004 7FFC 0000 0000 0000 0000"
	}
};

/* ===== SICN resources for navigation button icons (16x16) ===== */
/*
 * Nav button icons drawn via CopyBits, centered in 20x20 button rect.
 * IDs 270-275 replace polygon-drawn button glyphs.
 */

/* 270: Back - left-pointing filled triangle */
resource 'SICN' (270, "Back") {
	{
		$"0000 0000 0000 0020 0060 00E0 01E0 03E0"
		$"07E0 03E0 01E0 00E0 0060 0020 0000 0000"
	}
};

/* 271: Forward - right-pointing filled triangle */
resource 'SICN' (271, "Forward") {
	{
		$"0000 0000 0000 0400 0600 0700 0780 07C0"
		$"07E0 07C0 0780 0700 0600 0400 0000 0000"
	}
};

/* 272: Home - house with roof and door */
resource 'SICN' (272, "Home") {
	{
		$"0000 0180 03C0 07E0 0FF0 1FF8 3FFC 0FF0"
		$"0FF0 0FF0 0FF0 0E70 0E70 0000 0000 0000"
	}
};

/* 273: Stop - filled square */
resource 'SICN' (273, "Stop") {
	{
		$"0000 0000 0000 0000 0FF0 0FF0 0FF0 0FF0"
		$"0FF0 0FF0 0FF0 0FF0 0000 0000 0000 0000"
	}
};

/* 274: Go - right arrow with tail (navigate, distinct from Forward) */
resource 'SICN' (274, "Go") {
	{
		$"0000 0000 0000 0100 0180 01C0 7FE0 7FF0"
		$"7FE0 01C0 0180 0100 0000 0000 0000 0000"
	}
};

/* 275: Refresh - clockwise circular arrow */
resource 'SICN' (275, "Refresh") {
	{
		$"0000 0000 0780 1860 2010 2038 4008 4000"
		$"4000 2000 2008 1830 07C0 0000 0000 0000"
	}
};

/* ===== cicn color icon resources (Color QuickDraw systems) =====
 *
 * 1-bit cicn with custom foreground color. Same pixel design
 * as SICN but drawn in color via PlotCIcon on color systems.
 * Each cicn is 202 bytes: PixMap(50) + mask BM(14) + B&W BM(14)
 * + iconData(4) + mask(32) + B&W(32) + ctab(24) + pixels(32).
 *
 * Format: PixMap header | mask BitMap | B&W BitMap | iconData
 *         | mask data | B&W data | color table | pixel data
 */

/* cicn 256: Folder — blue */
data 'cicn' (256, "Folder") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 7E00 7FFC 4004 4004 4004 4004 4004 4004 4004 4004 7FFC 0000 0000 0000 0000"
	$"0000 7E00 7FFC 4004 4004 4004 4004 4004 4004 4004 4004 7FFC 0000 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 DDDD"
	$"0000 7E00 7FFC 4004 4004 4004 4004 4004 4004 4004 4004 7FFC 0000 0000 0000 0000"
};

/* cicn 257: Document — black */
data 'cicn' (257, "Document") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 3FC0 20F0 2010 2010 2010 2010 2010 2010 2010 2010 2010 3FF0 0000 0000 0000"
	$"0000 3FC0 20F0 2010 2010 2010 2010 2010 2010 2010 2010 2010 3FF0 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 0000"
	$"0000 3FC0 20F0 2010 2010 2010 2010 2010 2010 2010 2010 2010 3FF0 0000 0000 0000"
};

/* cicn 258: Search — blue */
data 'cicn' (258, "Search") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 7E00 7FFC 4004 4784 40C4 4184 4304 4004 4304 4004 7FFC 0000 0000 0000 0000"
	$"0000 7E00 7FFC 4004 4784 40C4 4184 4304 4004 4304 4004 7FFC 0000 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 DDDD"
	$"0000 7E00 7FFC 4004 4784 40C4 4184 4304 4004 4304 4004 7FFC 0000 0000 0000 0000"
};

/* cicn 259: Error — red */
data 'cicn' (259, "Error") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0100 0280 0280 0540 0540 0920 0920 1010 1110 2008 3FF8 0000 0000 0000 0000"
	$"0000 0100 0280 0280 0540 0540 0920 0920 1010 1110 2008 3FF8 0000 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 DDDD 0000 0000"
	$"0000 0100 0280 0280 0540 0540 0920 0920 1010 1110 2008 3FF8 0000 0000 0000 0000"
};

/* cicn 260: Binary — brown */
data 'cicn' (260, "Binary") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 7FF8 4008 4008 4008 4008 4008 7FF8 4008 4008 4008 4008 4008 7FF8 0000 0000"
	$"0000 7FF8 4008 4008 4008 4008 4008 7FF8 4008 4008 4008 4008 4008 7FF8 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 9999 6666 0000"
	$"0000 7FF8 4008 4008 4008 4008 4008 7FF8 4008 4008 4008 4008 4008 7FF8 0000 0000"
};

/* cicn 261: Terminal — green */
data 'cicn' (261, "Terminal") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 7FF8 4008 5808 4008 4008 4008 4008 4008 7FF8 0300 0FC0 0000 0000 0000 0000"
	$"0000 7FF8 4008 5808 4008 4008 4008 4008 4008 7FF8 0300 0FC0 0000 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 AAAA 0000"
	$"0000 7FF8 4008 5808 4008 4008 4008 4008 4008 7FF8 0300 0FC0 0000 0000 0000 0000"
};

/* cicn 262: Image — purple */
data 'cicn' (262, "Image") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 7FF8 4008 5808 4008 4008 4008 4048 40A8 4118 4008 7FF8 0000 0000 0000 0000"
	$"0000 7FF8 4008 5808 4008 4008 4008 4048 40A8 4118 4008 7FF8 0000 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 8888 0000 8888"
	$"0000 7FF8 4008 5808 4008 4008 4008 4048 40A8 4118 4008 7FF8 0000 0000 0000 0000"
};

/* cicn 263: Globe — teal */
data 'cicn' (263, "Globe") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0FC0 3330 4308 4308 4308 7FFC 4308 4308 4308 3330 0FC0 0000 0000 0000 0000"
	$"0000 0FC0 3330 4308 4308 4308 7FFC 4308 4308 4308 3330 0FC0 0000 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 8888 8888"
	$"0000 0FC0 3330 4308 4308 4308 7FFC 4308 4308 4308 3330 0FC0 0000 0000 0000 0000"
};

/* cicn 264: Speaker — magenta */
data 'cicn' (264, "Speaker") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0000 0400 0600 0700 3F10 3F28 3F44 3F28 3F10 0700 0600 0400 0000 0000 0000"
	$"0000 0000 0400 0600 0700 3F10 3F28 3F44 3F28 3F10 0700 0600 0400 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 9999 0000 6666"
	$"0000 0000 0400 0600 0700 3F10 3F28 3F44 3F28 3F10 0700 0600 0400 0000 0000 0000"
};

/* cicn 265: Phonebook — dark blue */
data 'cicn' (265, "Phonebook") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 7FF0 4010 5FD0 4010 5FD0 4010 5FD0 4010 5FD0 4010 5FD0 4010 7FF0 0000 0000"
	$"0000 7FF0 4010 5FD0 4010 5FD0 4010 5FD0 4010 5FD0 4010 5FD0 4010 7FF0 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 9999"
	$"0000 7FF0 4010 5FD0 4010 5FD0 4010 5FD0 4010 5FD0 4010 5FD0 4010 7FF0 0000 0000"
};

/* cicn 266: Unknown — gray */
data 'cicn' (266, "Unknown") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 7FF8 4008 4788 40C8 4188 4308 4308 4008 4308 4308 4008 7FF8 0000 0000 0000"
	$"0000 7FF8 4008 4788 40C8 4188 4308 4308 4008 4308 4308 4008 7FF8 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 8888 8888 8888"
	$"0000 7FF8 4008 4788 40C8 4188 4308 4308 4008 4308 4308 4008 7FF8 0000 0000 0000"
};

/* cicn 270: Back — dark blue */
data 'cicn' (270, "Back") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0000 0000 0020 0060 00E0 01E0 03E0 07E0 03E0 01E0 00E0 0060 0020 0000 0000"
	$"0000 0000 0000 0020 0060 00E0 01E0 03E0 07E0 03E0 01E0 00E0 0060 0020 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 CCCC"
	$"0000 0000 0000 0020 0060 00E0 01E0 03E0 07E0 03E0 01E0 00E0 0060 0020 0000 0000"
};

/* cicn 271: Forward — dark blue */
data 'cicn' (271, "Forward") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0000 0000 0400 0600 0700 0780 07C0 07E0 07C0 0780 0700 0600 0400 0000 0000"
	$"0000 0000 0000 0400 0600 0700 0780 07C0 07E0 07C0 0780 0700 0600 0400 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 CCCC"
	$"0000 0000 0000 0400 0600 0700 0780 07C0 07E0 07C0 0780 0700 0600 0400 0000 0000"
};

/* cicn 272: Home — dark blue (matches Back/Forward) */
data 'cicn' (272, "Home") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0180 03C0 07E0 0FF0 1FF8 3FFC 0FF0 0FF0 0FF0 0FF0 0E70 0E70 0000 0000 0000"
	$"0000 0180 03C0 07E0 0FF0 1FF8 3FFC 0FF0 0FF0 0FF0 0FF0 0E70 0E70 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 CCCC"
	$"0000 0180 03C0 07E0 0FF0 1FF8 3FFC 0FF0 0FF0 0FF0 0FF0 0E70 0E70 0000 0000 0000"
};

/* cicn 273: Stop — red */
data 'cicn' (273, "Stop") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0000 0000 0000 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0000 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 DDDD 0000 0000"
	$"0000 0000 0000 0000 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0FF0 0000 0000 0000 0000"
};

/* cicn 274: Go — green */
data 'cicn' (274, "Go") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0000 0000 0400 0600 0700 0780 07C0 07E0 07C0 0780 0700 0600 0400 0000 0000"
	$"0000 0000 0000 0400 0600 0700 0780 07C0 07E0 07C0 0780 0700 0600 0400 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 AAAA 0000"
	$"0000 0000 0000 0400 0600 0700 0780 07C0 07E0 07C0 0780 0700 0600 0400 0000 0000"
};

/* cicn 275: Refresh — blue */
data 'cicn' (275, "Refresh") {
	$"00000000 8002 0000 0000 0010 0010 0000 0000 00000000 00480000 00480000"
	$"0000 0001 0001 0001 00000000 00000000 00000000"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000 0002 0000 0000 0010 0010"
	$"00000000"
	$"0000 0000 0780 1860 2010 2038 4008 4000 4000 2000 2008 1830 07C0 0000 0000 0000"
	$"0000 0000 0780 1860 2010 2038 4008 4000 4000 2000 2008 1830 07C0 0000 0000 0000"
	$"00000000 0000 0001 0000 FFFF FFFF FFFF 0001 0000 0000 DDDD"
	$"0000 0000 0780 1860 2010 2038 4008 4000 4000 2000 2008 1830 07C0 0000 0000 0000"
};


/* Version resource — shown in Finder Get Info (System 7) */
resource 'vers' (1) {
	0x01, 0x01, release, 0x00, verUS,
	"1.0.1",
	"Geomys 1.0.1 \0xA9 2025\0x962026"
};

resource 'vers' (2) {
	0x01, 0x01, release, 0x00, verUS,
	"1.0.1",
	"Gopher browser for classic Macintosh"
};
