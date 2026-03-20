/*
 * geomys.r - Resources for Geomys Gopher browser
 */

#include "Menus.r"
#include "Dialogs.r"
#include "Processes.r"

resource 'MBAR' (128) {
	{ 128, 129, 130, 131, 133, 132 }
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
		"-", noIcon, noKey, noMark, plain;
		"Open URL\311", noIcon, "L", noMark, plain;
		"Save Page As\311", noIcon, "S", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Close Window", noIcon, "W", noMark, plain;
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
		"Select All", noIcon, "A", noMark, plain
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
		"-", noIcon, noKey, noMark, plain;
		"Font", noIcon, noKey, noMark, plain;
		"Page Style", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Show Details", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (133, "Window") {
	133, textMenuProc, allEnabled, enabled, "Window",
	{
		"1 of 4 Windows", noIcon, noKey, noMark, plain;
		"-", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (134, "Font") {
	134, textMenuProc, allEnabled, enabled, "Font",
	{
		"Monaco 9", noIcon, noKey, noMark, plain;
		"Monaco 12", noIcon, noKey, noMark, plain;
		"Chicago 12", noIcon, noKey, noMark, plain;
		"Courier 10", noIcon, noKey, noMark, plain;
		"Geneva 9", noIcon, noKey, noMark, plain;
		"Geneva 10", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (135, "Page Style") {
	135, textMenuProc, allEnabled, enabled, "Page Style",
	{
		"Traditional", noIcon, noKey, noMark, plain;
		"Plain", noIcon, noKey, noMark, plain;
		"Markdown", noIcon, noKey, noMark, plain
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
		StaticText { disabled, "Geomys 0.2.1" };

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

/* Open URL dialog */
resource 'DLOG' (131, "Open URL") {
	{80, 60, 175, 440},
	dBoxProc,
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
		EditText { enabled, "gopher://sdf.org" };

		/* 5: Default button outline (UserItem) */
		{56, 296, 84, 374},
		UserItem { disabled };
	}
};

/* Search query dialog */
resource 'DLOG' (132, "Search") {
	{90, 80, 185, 420},
	dBoxProc,
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

/* Home Page dialog */
resource 'DLOG' (133, "Home Page") {
	{80, 80, 195, 420},
	dBoxProc,
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
		EditText { enabled, "gopher://sdf.org" };

		/* 5: Use Blank checkbox */
		{45, 15, 61, 200},
		CheckBox { enabled, "Use Blank Page" };

		/* 6: Default button outline (UserItem) */
		{76, 246, 104, 324},
		UserItem { disabled };
	}
};

/* Favorites manager dialog */
resource 'DLOG' (134, "Favorites") {
	{40, 60, 300, 430},
	dBoxProc,
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

/* Edit/Add Favorite dialog */
resource 'DLOG' (135, "Edit Favorite") {
	{90, 90, 210, 420},
	dBoxProc,
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
	notHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	384 * 1024,
	256 * 1024
};
