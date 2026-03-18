/*
 * geomys.r - Resources for Geomys Gopher browser
 */

#include "Menus.r"
#include "Dialogs.r"
#include "Processes.r"

resource 'MBAR' (128) {
	{ 128, 129, 130, 131, 132 }
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
		"Open URL\311", noIcon, "L", noMark, plain;
		"-", noIcon, noKey, noMark, plain;
		"Close", noIcon, "W", noMark, plain;
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
		"Page Style", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (134, "Font") {
	134, textMenuProc, allEnabled, enabled, "Font",
	{
		"Monaco 9", noIcon, noKey, noMark, plain;
		"Geneva 9", noIcon, noKey, noMark, plain;
		"Chicago 12", noIcon, noKey, noMark, plain
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
		StaticText { disabled, "Geomys 0.1.0" };

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
data 'CURS' (129) {
	/* Cursor bitmap (16x16) — pointing hand */
	$"0600 0900 0900 0900"
	$"09C0 09B0 6DB8 9248"
	$"9008 8008 4008 2008"
	$"2010 1010 0820 0840"
	/* Mask bitmap (16x16) */
	$"0600 0F00 0F00 0F00"
	$"0FC0 0FF0 6FF8 FFF8"
	$"FFF8 FFF8 7FF8 3FF8"
	$"3FF0 1FF0 0FE0 0FC0"
	/* Hotspot */
	$"0001 0001"
};

/* Application icon - 32x32 bitmap for About dialog */
/* Pocket gopher head: round ears, eyes, prominent front teeth, whiskers */
data 'ICON' (128) {
	$"00000000"  /* row 0 */
	$"03800380"  /* row 1: ears (two small circles at top) */
	$"07C007C0"  /* row 2: ears wider */
	$"07C007C0"  /* row 3: ears */
	$"03E00F80"  /* row 4: ears meet head */
	$"01FFFF00"  /* row 5: head top */
	$"00FFFE00"  /* row 6 */
	$"007FFC00"  /* row 7: forehead */
	$"00FFFE00"  /* row 8: brow */
	$"01FFFF00"  /* row 9 */
	$"03CFFF80"  /* row 10: eyes (gaps in fill) */
	$"07C7FFC0"  /* row 11: eyes */
	$"0FE3FFE0"  /* row 12: cheeks */
	$"0FF1FFF0"  /* row 13 */
	$"1FFFFFF8"  /* row 14: wide cheeks */
	$"1FFFFFF8"  /* row 15 */
	$"0FFFFFF0"  /* row 16: nose area */
	$"07FFFFE0"  /* row 17 */
	$"03FFFFC0"  /* row 18: mouth area */
	$"01EFEE00"  /* row 19: whiskers (...|.|.|..) */
	$"00E7CE00"  /* row 20: teeth gap */
	$"00E3CE00"  /* row 21: front teeth (two rectangles) */
	$"00E3CE00"  /* row 22: teeth */
	$"00F7DE00"  /* row 23: chin */
	$"007FFC00"  /* row 24: jaw */
	$"003FF800"  /* row 25 */
	$"001FF000"  /* row 26 */
	$"000FE000"  /* row 27 */
	$"0007C000"  /* row 28 */
	$"00038000"  /* row 29 */
	$"00000000"  /* row 30 */
	$"00000000"  /* row 31 */
};

/* Application icon - 32x32 1-bit + mask */
data 'ICN#' (128) {
	/* Icon bitmap (same as ICON) */
	$"00000000"
	$"03800380"
	$"07C007C0"
	$"07C007C0"
	$"03E00F80"
	$"01FFFF00"
	$"00FFFE00"
	$"007FFC00"
	$"00FFFE00"
	$"01FFFF00"
	$"03CFFF80"
	$"07C7FFC0"
	$"0FE3FFE0"
	$"0FF1FFF0"
	$"1FFFFFF8"
	$"1FFFFFF8"
	$"0FFFFFF0"
	$"07FFFFE0"
	$"03FFFFC0"
	$"01EFEE00"
	$"00E7CE00"
	$"00E3CE00"
	$"00E3CE00"
	$"00F7DE00"
	$"007FFC00"
	$"003FF800"
	$"001FF000"
	$"000FE000"
	$"0007C000"
	$"00038000"
	$"00000000"
	$"00000000"
	/* Mask bitmap (silhouette) */
	$"00000000"
	$"03800380"
	$"07C007C0"
	$"07C007C0"
	$"03E00F80"
	$"01FFFF00"
	$"00FFFE00"
	$"007FFC00"
	$"00FFFE00"
	$"01FFFF00"
	$"03FFFF80"
	$"07FFFFC0"
	$"0FFFFFE0"
	$"0FFFFFF0"
	$"1FFFFFF8"
	$"1FFFFFF8"
	$"0FFFFFF0"
	$"07FFFFE0"
	$"03FFFFC0"
	$"01FFFF00"
	$"00FFFE00"
	$"00FFFE00"
	$"00FFFE00"
	$"00FFFE00"
	$"007FFC00"
	$"003FF800"
	$"001FF000"
	$"000FE000"
	$"0007C000"
	$"00038000"
	$"00000000"
	$"00000000"
};

/* Application icon - 16x16 1-bit + mask */
/* Pocket gopher mini: ears, eyes, teeth */
data 'ics#' (128) {
	/* Icon bitmap */
	$"4100"  /* .#....#. — ears */
	$"6300"  /* .##...## — ears */
	$"3E00"  /* ..#####. — head top */
	$"7F00"  /* .####### — head */
	$"7700"  /* .###.### — eyes */
	$"7F00"  /* .####### — cheeks */
	$"3E00"  /* ..#####. — nose */
	$"1C00"  /* ...###.. — mouth */
	$"1400"  /* ...#.#.. — teeth */
	$"1400"  /* ...#.#.. — teeth */
	$"1C00"  /* ...###.. — chin */
	$"0800"  /* ....#... — neck */
	$"0000"
	$"0000"
	$"0000"
	$"0000"
	/* Mask bitmap */
	$"4100"
	$"6300"
	$"3E00"
	$"7F00"
	$"7F00"
	$"7F00"
	$"3E00"
	$"1C00"
	$"1C00"
	$"1C00"
	$"1C00"
	$"0800"
	$"0000"
	$"0000"
	$"0000"
	$"0000"
};

/* File reference - APPL type, icon 0 */
data 'FREF' (128) {
	$"4150 504C 0000 00"                                  /* APPL... */
};

/* Bundle - associates creator 'GEOM' with icons and file refs */
data 'BNDL' (128) {
	$"4745 4F4D"                                          /* GEOM */
	$"0000"                                               /* owner ID */
	$"0001"                                               /* 2 types */
	$"4652 4546"                                          /* FREF */
	$"0000"                                               /* 1 entry */
	$"0000 0080"                                          /* local 0 -> res 128 (APPL) */
	$"4943 4E23"                                          /* ICN# */
	$"0000"                                               /* 1 entry */
	$"0000 0080"                                          /* local 0 -> res 128 (app icon) */
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
