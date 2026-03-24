/*
 * geomys.r - Resources for Geomys Gopher browser
 */

#include "Menus.r"
#include "Dialogs.r"
#include "Processes.r"
#include "Balloons.r"

resource 'MBAR' (128) {
	{ 128, 129, 130, 134, 131, 132, 133 }
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
		"Close Window", noIcon, "W", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Save Page As\311", noIcon, "S", noMark, plain;
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
		"Find Again", noIcon, "G", noMark, plain
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
		"Stop", noIcon, ".", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Open Location\311", noIcon, "L", noMark, plain;
	}
};

resource 'MENU' (131, "Favorites") {
	131, textMenuProc, allEnabled, enabled, "Favorites",
	{
		"Manage Favorites\311", noIcon, noKey, noMark, plain;
		"Add Favorite\311", noIcon, "D", noMark, plain
	}
};

resource 'MENU' (132, "Options") {
	132, textMenuProc, allEnabled, enabled, "Options",
	{
		"Home Page\311", noIcon, noKey, noMark, plain;
		"DNS Server\311", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Font", noIcon, noKey, noMark, plain;
		"Page Style", noIcon, noKey, noMark, plain;
		"Theme", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Show Details", noIcon, noKey, noMark, plain;
		"Status Bar", noIcon, noKey, noMark, plain
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
		"Monaco 9", noIcon, noKey, noMark, plain;
		"Monaco 12", noIcon, noKey, noMark, plain;
		"Chicago 12", noIcon, noKey, noMark, plain;
		"Courier 10", noIcon, noKey, noMark, plain;
		"Geneva 9", noIcon, noKey, noMark, plain;
		"Geneva 10", noIcon, noKey, noMark, plain
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
		"Tokyo Night Light", noIcon, noKey, noMark, plain;
		"Tokyo Night Dark", noIcon, noKey, noMark, plain;
		"Green Screen", noIcon, noKey, noMark, plain;
		"Classic", noIcon, noKey, noMark, plain;
		"Platinum", noIcon, noKey, noMark, plain
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
		StaticText { disabled, "Geomys 0.11.0" };

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
		StaticText { disabled, "https://github.com/ecliptik/geomys" };

		/* 8: Default button outline (UserItem) */
		{134, 111, 162, 189},
		UserItem { disabled };
	}
};

/* Open URL dialog — movableDBoxProc for System 7 (falls back on System 6) */
resource 'DLOG' (131, "Open URL") {
	{80, 60, 175, 440},
	movableDBoxProc,
	visible,
	noGoAway,
	0x0,
	131,
	"Open URL",
	noAutoCenter
};

resource 'DITL' (131, "Open URL") {
	{
		/* 1: Connect button */
		{60, 300, 80, 370},
		Button { enabled, "Connect" };

		/* 2: Cancel button */
		{60, 215, 80, 285},
		Button { enabled, "Cancel" };

		/* 3: URL label */
		{15, 15, 31, 75},
		StaticText { disabled, "URL:" };

		/* 4: URL field */
		{15, 80, 31, 365},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{56, 296, 84, 374},
		UserItem { disabled };
	}
};

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

		/* 3: Label (set at runtime to show search item name) */
		{15, 15, 31, 320},
		StaticText { disabled, "Search for:" };

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
		StaticText { disabled, "Look up in:" };

		/* 4: Query field */
		{35, 15, 51, 320},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{56, 241, 84, 324},
		UserItem { disabled };
	}
};

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
		StaticText { disabled, "Find:" };

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
		StaticText { disabled, "This link points to a web page outside of Gopher:" };

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
	{80, 80, 195, 420},
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
		{80, 250, 100, 320},
		Button { enabled, "OK" };

		/* 2: Cancel button */
		{80, 160, 100, 230},
		Button { enabled, "Cancel" };

		/* 3: URL label */
		{15, 15, 31, 100},
		StaticText { disabled, "Home Page:" };

		/* 4: URL field */
		{15, 105, 31, 325},
		EditText { enabled, "" };

		/* 5: Use Blank checkbox */
		{45, 15, 61, 200},
		CheckBox { enabled, "Use Blank Page" };

		/* 6: Default button outline (UserItem) */
		{76, 246, 104, 324},
		UserItem { disabled };
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

		/* 7: Delete button */
		{75, 280, 95, 350},
		Button { enabled, "Delete" };

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
		StaticText { disabled, "Name:" };

		/* 4: Name field */
		{15, 75, 31, 315},
		EditText { enabled, "" };

		/* 5: URL label */
		{45, 15, 61, 70},
		StaticText { disabled, "URL:" };

		/* 6: URL field */
		{45, 75, 61, 315},
		EditText { enabled, "" };

		/* 7: Default button outline (UserItem) */
		{81, 241, 109, 319},
		UserItem { disabled };
	}
};

/* Delete confirmation alert — Cancel is default (item 1) per HIG */
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

resource 'DITL' (129, "Delete Confirm") {
	{
		/* 1: Cancel button (default — safe action) */
		{85, 250, 105, 320},
		Button { enabled, "Cancel" };

		/* 2: Delete button (destructive) */
		{85, 140, 105, 210},
		Button { enabled, "Delete" };

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
		StaticText { disabled, "Host:" };

		/* 6: Host value (editable for Cmd-C) */
		{38, 60, 54, 345},
		EditText { enabled, "" };

		/* 7: Port label */
		{62, 15, 78, 55},
		StaticText { disabled, "Port:" };

		/* 8: Port value */
		{62, 60, 78, 120},
		StaticText { disabled, "" };

		/* 9: Login label */
		{86, 15, 102, 55},
		StaticText { disabled, "Login:" };

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
		StaticText { disabled, "DNS Server:" };

		/* 4: DNS Server IP field */
		{15, 115, 31, 275},
		EditText { enabled, "" };

		/* 5: Default button outline (UserItem) */
		{56, 206, 84, 284},
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
	notStationeryAware,
	useTextEditServices,
	reserved,
	reserved,
	reserved,
	384 * 1024,
	256 * 1024
};

/* ===== Balloon Help resources for all menus (System 7) ===== */

/*
 * Apple Menu (128)
 * HMSkipItem for missing = no balloon on appended DA items
 */
resource 'hmnu' (128, "Apple") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"This menu lists desk accessories and "
			"provides information about Geomys.",
			"", "", ""
		},
		/* About Geomys... */
		HMStringItem {
			"Displays version, copyright, and system "
			"information for Geomys.",
			"", "", ""
		}
	}
};

/*
 * File Menu (129)
 * Items: New Window(1), Close Window(2), -(3),
 *        Save Page As...(4), Page Setup...(5), Print...(6),
 *        -(7), Quit(8)
 */
resource 'hmnu' (129, "File") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"File menu\nUse this menu to manage "
			"windows, save pages, print, and quit.",
			"", "", ""
		},
		/* 1: New Window */
		HMStringItem {
			"Opens a new Gopher browser window.",
			"Not available because the maximum "
			"number of windows are already open.",
			"", ""
		},
		/* 2: Close Window */
		HMStringItem {
			"Closes the active browser window.",
			"Not available because no windows "
			"are open.",
			"", ""
		},
		/* 3: separator */
		HMSkipItem { },
		/* 4: Save Page As... */
		HMStringItem {
			"Saves the current page to a file "
			"on disk.",
			"Not available because no page "
			"is loaded.",
			"", ""
		},
		/* 5: Page Setup... */
		HMStringItem {
			"Configures page layout options "
			"for printing.",
			"Not available because printing "
			"is not enabled.",
			"", ""
		},
		/* 6: Print... */
		HMStringItem {
			"Prints the current page.",
			"Not available because no page is "
			"loaded or a page is still loading.",
			"", ""
		},
		/* 7: separator */
		HMSkipItem { },
		/* 8: Quit */
		HMStringItem {
			"Quits Geomys.",
			"", "", ""
		}
	}
};

/*
 * Edit Menu (130)
 * Items: Undo(1), -(2), Cut(3), Copy(4), Paste(5), Clear(6),
 *        -(7), Select All(8), -(9), Find...(10), Find Again(11)
 */
resource 'hmnu' (130, "Edit") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Edit menu\nUse this menu to undo changes, "
			"work with the Clipboard, and find text.",
			"", "", ""
		},
		/* 1: Undo */
		HMStringItem {
			"Cancels your last action.",
			"Not available because there is "
			"nothing to undo.",
			"", ""
		},
		/* 2: separator */
		HMSkipItem { },
		/* 3: Cut */
		HMStringItem {
			"Removes the selected text and places "
			"it on the Clipboard.",
			"Not available because no text "
			"is selected.",
			"", ""
		},
		/* 4: Copy */
		HMStringItem {
			"Copies the selected text to "
			"the Clipboard.",
			"Not available because no text "
			"is selected.",
			"", ""
		},
		/* 5: Paste */
		HMStringItem {
			"Places the contents of the Clipboard "
			"at the insertion point.",
			"Not available because the Clipboard "
			"is empty or no text field is active.",
			"", ""
		},
		/* 6: Clear */
		HMStringItem {
			"Removes the selected text without "
			"placing it on the Clipboard.",
			"Not available because no text "
			"is selected.",
			"", ""
		},
		/* 7: separator */
		HMSkipItem { },
		/* 8: Select All */
		HMStringItem {
			"Selects all text in the active area.",
			"Not available because there is "
			"nothing to select.",
			"", ""
		},
		/* 9: separator */
		HMSkipItem { },
		/* 10: Find... */
		HMStringItem {
			"Searches for text on the current page.",
			"Not available because no page "
			"is loaded.",
			"", ""
		},
		/* 11: Find Again */
		HMStringItem {
			"Finds the next occurrence of the "
			"search text.",
			"Not available because no previous "
			"search has been made.",
			"", ""
		}
	}
};

/*
 * Go Menu (134)
 * Items: Back(1), Forward(2), Home(3), -(4),
 *        Refresh(5), Stop(6), -(7), Open Location...(8)
 */
resource 'hmnu' (134, "Go") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Go menu\nUse this menu to navigate "
			"between Gopher pages.",
			"", "", ""
		},
		/* 1: Back */
		HMStringItem {
			"Goes to the previous page in your "
			"browsing history.",
			"Not available because there is "
			"no previous page.",
			"", ""
		},
		/* 2: Forward */
		HMStringItem {
			"Goes to the next page in your "
			"browsing history.",
			"Not available because there is "
			"no next page.",
			"", ""
		},
		/* 3: Home */
		HMStringItem {
			"Goes to your home page.",
			"Not available because no home "
			"page is set.",
			"", ""
		},
		/* 4: separator */
		HMSkipItem { },
		/* 5: Refresh */
		HMStringItem {
			"Reloads the current page from "
			"the server.",
			"Not available because no page "
			"is loaded.",
			"", ""
		},
		/* 6: Stop */
		HMStringItem {
			"Stops loading the current page.",
			"Not available because no page "
			"is currently loading.",
			"", ""
		},
		/* 7: separator */
		HMSkipItem { },
		/* 8: Open Location... */
		HMStringItem {
			"Lets you type a Gopher URL to visit.",
			"", "", ""
		}
	}
};

/*
 * Favorites Menu (131)
 * Items: Manage Favorites...(1), Add Favorite...(2)
 * Dynamic favorites appended — HMSkipItem covers those
 */
resource 'hmnu' (131, "Favorites") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Favorites menu\nUse this menu to save "
			"and visit your favorite Gopher pages.",
			"", "", ""
		},
		/* 1: Manage Favorites... */
		HMStringItem {
			"Opens a window to organize, edit, and "
			"remove your saved favorites.",
			"", "", ""
		},
		/* 2: Add Favorite... */
		HMStringItem {
			"Saves the current page as a favorite.",
			"", "", ""
		}
	}
};

/*
 * Options Menu (132)
 * Items: Home Page...(1), DNS Server...(2), -(3),
 *        Font(4), Page Style(5), Theme(6), -(7),
 *        Show Details(8), Status Bar(9)
 */
resource 'hmnu' (132, "Options") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Options menu\nUse this menu to change "
			"browser settings and appearance.",
			"", "", ""
		},
		/* 1: Home Page... */
		HMStringItem {
			"Sets the page that opens when you "
			"choose Home from the Go menu.",
			"", "", ""
		},
		/* 2: DNS Server... */
		HMStringItem {
			"Sets the DNS server address used "
			"for name resolution.",
			"", "", ""
		},
		/* 3: separator */
		HMSkipItem { },
		/* 4: Font (hierarchical submenu) */
		HMStringItem {
			"Changes the font and size used to "
			"display page content.",
			"", "", ""
		},
		/* 5: Page Style (hierarchical submenu) */
		HMStringItem {
			"Switches between text and icon display "
			"modes for Gopher directories.",
			"", "", ""
		},
		/* 6: Theme (hierarchical submenu) */
		HMStringItem {
			"Changes the color scheme of "
			"the browser.",
			"", "", ""
		},
		/* 7: separator */
		HMSkipItem { },
		/* 8: Show Details (toggle — checkmark) */
		HMStringItem {
			"Shows Gopher type codes and port "
			"numbers for each item.",
			"",
			"Hides Gopher type codes and port "
			"numbers. Details are currently shown.",
			""
		},
		/* 9: Status Bar (toggle — checkmark) */
		HMStringItem {
			"Shows the status bar at the bottom "
			"of the window.",
			"",
			"Hides the status bar. The status bar "
			"is currently shown.",
			""
		}
	}
};

/*
 * Window Menu (133)
 * Items: "1 of 4 Windows"(1), -(2)
 * Dynamic window items appended — HMSkipItem covers those
 */
resource 'hmnu' (133, "Window") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Window menu\nUse this menu to switch "
			"between open browser windows.",
			"", "", ""
		},
		/* 1: Window count display */
		HMStringItem {
			"Shows the number of open windows.",
			"Shows the number of open windows.",
			"", ""
		}
	}
};

/*
 * Font Submenu (135)
 * Items: Monaco 9(1), Monaco 12(2), Chicago 12(3),
 *        Courier 10(4), Geneva 9(5), Geneva 10(6)
 */
resource 'hmnu' (135, "Font") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Font submenu\nChoose a font and size "
			"for displaying page content.",
			"", "", ""
		},
		/* 1: Monaco 9 */
		HMStringItem {
			"Displays page content in Monaco "
			"9-point.",
			"",
			"Monaco 9-point is the current font.",
			""
		},
		/* 2: Monaco 12 */
		HMStringItem {
			"Displays page content in Monaco "
			"12-point.",
			"",
			"Monaco 12-point is the current font.",
			""
		},
		/* 3: Chicago 12 */
		HMStringItem {
			"Displays page content in Chicago "
			"12-point.",
			"",
			"Chicago 12-point is the current font.",
			""
		},
		/* 4: Courier 10 */
		HMStringItem {
			"Displays page content in Courier "
			"10-point.",
			"",
			"Courier 10-point is the current font.",
			""
		},
		/* 5: Geneva 9 */
		HMStringItem {
			"Displays page content in Geneva "
			"9-point.",
			"",
			"Geneva 9-point is the current font.",
			""
		},
		/* 6: Geneva 10 */
		HMStringItem {
			"Displays page content in Geneva "
			"10-point.",
			"",
			"Geneva 10-point is the current font.",
			""
		}
	}
};

/*
 * Page Style Submenu (136)
 * Items: Text(1), Icons(2)
 */
resource 'hmnu' (136, "Page Style") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Page Style submenu\nChoose how Gopher "
			"directory items are displayed.",
			"", "", ""
		},
		/* 1: Text */
		HMStringItem {
			"Displays directory items as a text list.",
			"",
			"Directory items are displayed as text. "
			"This is the current style.",
			""
		},
		/* 2: Icons */
		HMStringItem {
			"Displays directory items with icons.",
			"",
			"Directory items are displayed with icons. "
			"This is the current style.",
			""
		}
	}
};

/*
 * Theme Submenu (137)
 * Items: Light(1), Dark(2), -(3), Solarized Light(4),
 *        Solarized Dark(5), Tokyo Night Light(6),
 *        Tokyo Night Dark(7), Green Screen(8),
 *        Classic(9), Platinum(10)
 */
resource 'hmnu' (137, "Theme") {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem { },
	{
		/* Menu title */
		HMStringItem {
			"Theme submenu\nChoose a color scheme "
			"for the browser.",
			"", "", ""
		},
		/* 1: Light */
		HMStringItem {
			"Uses the Light color scheme with "
			"dark text on a white background.",
			"",
			"The Light theme is currently active.",
			""
		},
		/* 2: Dark */
		HMStringItem {
			"Uses the Dark color scheme with "
			"light text on a dark background.",
			"",
			"The Dark theme is currently active.",
			""
		},
		/* 3: separator */
		HMSkipItem { },
		/* 4: Solarized Light */
		HMStringItem {
			"Uses the Solarized Light color scheme.",
			"Not available on monochrome displays.",
			"The Solarized Light theme is "
			"currently active.",
			""
		},
		/* 5: Solarized Dark */
		HMStringItem {
			"Uses the Solarized Dark color scheme.",
			"Not available on monochrome displays.",
			"The Solarized Dark theme is "
			"currently active.",
			""
		},
		/* 6: Tokyo Night Light */
		HMStringItem {
			"Uses the Tokyo Night Light "
			"color scheme.",
			"Not available on monochrome displays.",
			"The Tokyo Night Light theme is "
			"currently active.",
			""
		},
		/* 7: Tokyo Night Dark */
		HMStringItem {
			"Uses the Tokyo Night Dark "
			"color scheme.",
			"Not available on monochrome displays.",
			"The Tokyo Night Dark theme is "
			"currently active.",
			""
		},
		/* 8: Green Screen */
		HMStringItem {
			"Uses the Green Screen color scheme "
			"with green text on a black background.",
			"Not available on monochrome displays.",
			"The Green Screen theme is "
			"currently active.",
			""
		},
		/* 9: Classic */
		HMStringItem {
			"Uses the Classic color scheme "
			"inspired by original Macintosh.",
			"Not available on monochrome displays.",
			"The Classic theme is currently active.",
			""
		},
		/* 10: Platinum */
		HMStringItem {
			"Uses the Platinum color scheme "
			"inspired by Mac OS 8.",
			"Not available on monochrome displays.",
			"The Platinum theme is currently active.",
			""
		}
	}
};

/* Version resource — shown in Finder Get Info (System 7) */
resource 'vers' (1) {
	0x00, 0x0B, release, 0x00, verUS,
	"0.11.0",
	"Geomys 0.11.0 \0xA9 2025\0x962026"
};

resource 'vers' (2) {
	0x00, 0x0B, release, 0x00, verUS,
	"0.11.0",
	"Gopher browser for classic Macintosh"
};
