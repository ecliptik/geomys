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
		"Font", noIcon, noKey, noMark, plain
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

/* Application icon - 32x32 bitmap for About dialog */
/* Gopher (pocket gopher) silhouette - rounded body with small ears */
data 'ICON' (128) {
	$"00000000 00180000"
	$"003C0000 007E0000"
	$"00FF8000 03FFC000"
	$"07FFE000 0FFFF000"
	$"1FFFF800 1FFFFC00"
	$"3FFFFC00 3FFFFE00"
	$"3FFFFE00 3FFFFE00"
	$"3FFFFE00 3FFFFE00"
	$"1FFFFC00 1FFFFC00"
	$"0FFFF800 07FFF000"
	$"03FFE000 01FFC000"
	$"00FF8000 007F0000"
	$"003E0000 001C0000"
	$"00000000 00000000"
	$"00000000 00000000"
	$"00000000 00000000"
};

/* Application icon - 32x32 1-bit + mask */
data 'ICN#' (128) {
	/* Icon bitmap */
	$"00000000 00180000"
	$"003C0000 007E0000"
	$"00FF8000 03FFC000"
	$"07FFE000 0FFFF000"
	$"1FFFF800 1FFFFC00"
	$"3FFFFC00 3FFFFE00"
	$"3FFFFE00 3FFFFE00"
	$"3FFFFE00 3FFFFE00"
	$"1FFFFC00 1FFFFC00"
	$"0FFFF800 07FFF000"
	$"03FFE000 01FFC000"
	$"00FF8000 007F0000"
	$"003E0000 001C0000"
	$"00000000 00000000"
	$"00000000 00000000"
	$"00000000 00000000"
	/* Mask bitmap */
	$"00000000 00180000"
	$"003C0000 007E0000"
	$"00FF8000 03FFC000"
	$"07FFE000 0FFFF000"
	$"1FFFF800 1FFFFC00"
	$"3FFFFC00 3FFFFE00"
	$"3FFFFE00 3FFFFE00"
	$"3FFFFE00 3FFFFE00"
	$"1FFFFC00 1FFFFC00"
	$"0FFFF800 07FFF000"
	$"03FFE000 01FFC000"
	$"00FF8000 007F0000"
	$"003E0000 001C0000"
	$"00000000 00000000"
	$"00000000 00000000"
	$"00000000 00000000"
};

/* Application icon - 16x16 1-bit + mask */
data 'ics#' (128) {
	$"0180 03C0 07E0 0FF0 1FF8 3FFC 3FFC 3FFC"
	$"3FFC 1FF8 0FF0 07E0 03C0 0180 0000 0000"
	$"0180 03C0 07E0 0FF0 1FF8 3FFC 3FFC 3FFC"
	$"3FFC 1FF8 0FF0 07E0 03C0 0180 0000 0000"
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
