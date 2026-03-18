/*
 * menus.c - Menu management for Geomys
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <Menus.h>
#include <Dialogs.h>
#include <Resources.h>
#include <ToolUtils.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "menus.h"
#include "dialogs.h"
#include "macutil.h"

/* Menu handles (private to this module) */
static MenuHandle apple_menu, file_menu, edit_menu;
static MenuHandle favorites_menu, options_menu;
static MenuHandle font_submenu;

/* main.c globals accessed via main.h */

void
init_menus(void)
{
	Handle mbar;

	mbar = GetNewMBar(MBAR_ID);
	if (!mbar) {
		SysBeep(10);
		ExitToShell();
	}

	SetMenuBar(mbar);
	DisposeHandle(mbar);

	apple_menu = GetMenuHandle(APPLE_MENU_ID);
	if (apple_menu)
		AppendResMenu(apple_menu, 'DRVR');

	file_menu = GetMenuHandle(FILE_MENU_ID);
	edit_menu = GetMenuHandle(EDIT_MENU_ID);
	favorites_menu = GetMenuHandle(FAVORITES_MENU_ID);
	options_menu = GetMenuHandle(OPTIONS_MENU_ID);

	/* Load and insert Font hierarchical submenu */
	font_submenu = GetMenu(FONT_MENU_ID);
	if (font_submenu)
		InsertMenu(font_submenu, -1);

	/* Set hierarchical menu marker for Font item in Options */
	if (options_menu) {
		SetItemCmd(options_menu, OPT_MENU_FONT, 0x1B);
		SetItemMark(options_menu, OPT_MENU_FONT,
		    FONT_MENU_ID);
	}

	/* Disable Favorites menu items for now (Phase 7) */
	if (favorites_menu) {
		DisableItem(favorites_menu, FAV_MENU_MANAGE);
		DisableItem(favorites_menu, FAV_MENU_ADD);
	}

	DrawMenuBar();
}

void
update_menus(void)
{
	WindowPtr front;
	Boolean da_active;

	front = FrontWindow();
	/* DA windows have negative windowKind */
	da_active = (front && ((WindowPeek)front)->windowKind < 0);

	/* File menu: Open URL always enabled, Close when window open */
	if (file_menu) {
		EnableItem(file_menu, FILE_MENU_OPEN_URL);
		if (g_window)
			EnableItem(file_menu, FILE_MENU_CLOSE);
		else
			DisableItem(file_menu, FILE_MENU_CLOSE);
		EnableItem(file_menu, FILE_MENU_QUIT);
	}

	/* Edit menu: enable when DA is active so SystemEdit works,
	 * disable otherwise until we have text editing (Phase 3+) */
	if (edit_menu) {
		if (da_active) {
			EnableItem(edit_menu, EDIT_MENU_UNDO);
			EnableItem(edit_menu, EDIT_MENU_CUT);
			EnableItem(edit_menu, EDIT_MENU_COPY);
			EnableItem(edit_menu, EDIT_MENU_PASTE);
			EnableItem(edit_menu, EDIT_MENU_CLEAR);
			EnableItem(edit_menu, EDIT_MENU_SELALL);
		} else {
			DisableItem(edit_menu, EDIT_MENU_UNDO);
			DisableItem(edit_menu, EDIT_MENU_CUT);
			DisableItem(edit_menu, EDIT_MENU_COPY);
			DisableItem(edit_menu, EDIT_MENU_PASTE);
			DisableItem(edit_menu, EDIT_MENU_CLEAR);
			DisableItem(edit_menu, EDIT_MENU_SELALL);
		}
	}
}

static void
handle_apple_menu(short item)
{
	if (item == APPLE_MENU_ABOUT) {
		do_about();
	} else {
		Str255 da_name;
		GrafPtr save_port;

		GetMenuItemText(apple_menu, item, da_name);
		GetPort(&save_port);
		OpenDeskAcc(da_name);
		SetPort(save_port);
	}
}

static void
handle_file_menu(short item)
{
	switch (item) {
	case FILE_MENU_OPEN_URL:
		do_open_url_dialog();
		break;
	case FILE_MENU_CLOSE:
		g_running = false;
		break;
	case FILE_MENU_QUIT:
		g_running = false;
		break;
	}
}

static void
handle_edit_menu(short item)
{
	/* Let DA get edit commands first */
	if (!SystemEdit(item - 1)) {
		/* App-specific edit handling — Phase 3+ */
	}
}

Boolean
handle_menu(long menu_id)
{
	short menu, item;
	Boolean handled = true;

	menu = HiWord(menu_id);
	item = LoWord(menu_id);

	switch (menu) {
	case APPLE_MENU_ID:
		handle_apple_menu(item);
		break;
	case FILE_MENU_ID:
		handle_file_menu(item);
		break;
	case EDIT_MENU_ID:
		handle_edit_menu(item);
		break;
	case FAVORITES_MENU_ID:
		/* Phase 7 */
		break;
	case OPTIONS_MENU_ID:
		/* Phase 7 */
		break;
	case FONT_MENU_ID:
		/* Phase 7 */
		break;
	default:
		handled = false;
		break;
	}

	HiliteMenu(0);
	return handled;
}
