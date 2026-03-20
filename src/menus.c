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
#include "browser.h"
#include "dialogs.h"
#include "macutil.h"
#include "settings.h"
#include "favorites.h"
#include "gopher.h"
#include "content.h"
#ifdef GEOMYS_CLIPBOARD
#include "clipboard.h"
#endif

/* Menu handles (private to this module) */
static MenuHandle apple_menu, file_menu, edit_menu;
static MenuHandle favorites_menu, options_menu;
static MenuHandle font_submenu;
static MenuHandle style_submenu;

/* main.c globals */
extern GeomysPrefs g_prefs;

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

	/* Load and insert Page Style hierarchical submenu */
	style_submenu = GetMenu(STYLE_MENU_ID);
	if (style_submenu)
		InsertMenu(style_submenu, -1);

	/* Set hierarchical menu markers for Font and Style in Options */
	if (options_menu) {
		SetItemCmd(options_menu, OPT_MENU_FONT, 0x1B);
		SetItemMark(options_menu, OPT_MENU_FONT,
		    FONT_MENU_ID);
		SetItemCmd(options_menu, OPT_MENU_STYLE, 0x1B);
		SetItemMark(options_menu, OPT_MENU_STYLE,
		    STYLE_MENU_ID);
	}

	/* Set initial style checkmark based on prefs */
	if (style_submenu) {
		CheckItem(style_submenu, STYLE_ITEM_TRADITIONAL,
		    g_prefs.page_style == STYLE_TRADITIONAL);
		CheckItem(style_submenu, STYLE_ITEM_PLAIN,
		    g_prefs.page_style == STYLE_PLAIN);
		CheckItem(style_submenu, STYLE_ITEM_MARKDOWN,
		    g_prefs.page_style == STYLE_MARKDOWN);
	}

	/* Set initial font checkmark based on prefs */
	if (font_submenu) {
		CheckItem(font_submenu, FONT_MONACO9,
		    g_prefs.font_id == 4 && g_prefs.font_size == 9);
		CheckItem(font_submenu, FONT_MONACO12,
		    g_prefs.font_id == 4 && g_prefs.font_size == 12);
		CheckItem(font_submenu, FONT_COURIER10,
		    g_prefs.font_id == 22 && g_prefs.font_size == 10);
		CheckItem(font_submenu, FONT_CHICAGO12,
		    g_prefs.font_id == 0 && g_prefs.font_size == 12);
		CheckItem(font_submenu, FONT_GENEVA9,
		    g_prefs.font_id == 3 && g_prefs.font_size == 9);
		CheckItem(font_submenu, FONT_GENEVA10,
		    g_prefs.font_id == 3 && g_prefs.font_size == 10);
	}

	/* Set initial Show Details checkmark */
	if (options_menu)
		CheckItem(options_menu, OPT_MENU_DETAILS,
		    g_prefs.show_details != 0);

	/* Favorites menu — managed by favorites.c */

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
	 * or based on focus for clipboard operations */
	if (edit_menu) {
		if (da_active) {
			EnableItem(edit_menu, EDIT_MENU_UNDO);
			EnableItem(edit_menu, EDIT_MENU_CUT);
			EnableItem(edit_menu, EDIT_MENU_COPY);
			EnableItem(edit_menu, EDIT_MENU_PASTE);
			EnableItem(edit_menu, EDIT_MENU_CLEAR);
			EnableItem(edit_menu, EDIT_MENU_SELALL);
#ifdef GEOMYS_CLIPBOARD
		} else if (browser_get_focus() == FOCUS_ADDR_BAR) {
			DisableItem(edit_menu, EDIT_MENU_UNDO);
			if (browser_has_selection()) {
				EnableItem(edit_menu, EDIT_MENU_CUT);
				EnableItem(edit_menu, EDIT_MENU_COPY);
				EnableItem(edit_menu, EDIT_MENU_CLEAR);
			} else {
				DisableItem(edit_menu, EDIT_MENU_CUT);
				DisableItem(edit_menu, EDIT_MENU_COPY);
				DisableItem(edit_menu, EDIT_MENU_CLEAR);
			}
			EnableItem(edit_menu, EDIT_MENU_PASTE);
			EnableItem(edit_menu, EDIT_MENU_SELALL);
		} else if (browser_get_focus() == FOCUS_CONTENT) {
			DisableItem(edit_menu, EDIT_MENU_UNDO);
			DisableItem(edit_menu, EDIT_MENU_CUT);
			if (content_has_selection())
				EnableItem(edit_menu, EDIT_MENU_COPY);
			else
				DisableItem(edit_menu, EDIT_MENU_COPY);
			DisableItem(edit_menu, EDIT_MENU_PASTE);
			DisableItem(edit_menu, EDIT_MENU_CLEAR);
			EnableItem(edit_menu, EDIT_MENU_SELALL);
#endif
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
	if (SystemEdit(item - 1))
		return;

#ifdef GEOMYS_CLIPBOARD
	if (browser_get_focus() == FOCUS_ADDR_BAR) {
		switch (item) {
		case EDIT_MENU_CUT:
			browser_edit_cut();
			break;
		case EDIT_MENU_COPY:
			browser_edit_copy();
			break;
		case EDIT_MENU_PASTE:
			browser_edit_paste();
			break;
		case EDIT_MENU_CLEAR:
			browser_edit_clear();
			break;
		case EDIT_MENU_SELALL:
			browser_edit_select_all();
			break;
		}
	} else if (browser_get_focus() == FOCUS_CONTENT) {
		switch (item) {
		case EDIT_MENU_COPY:
			clipboard_copy_content(g_window);
			break;
		case EDIT_MENU_SELALL:
			clipboard_select_all(g_window);
			break;
		}
	}
#endif
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
#ifdef GEOMYS_FAVORITES
	case FAVORITES_MENU_ID:
		switch (item) {
		case FAV_MENU_MANAGE:
			favorites_manage(&g_prefs);
			break;
		case FAV_MENU_ADD: {
			char url[300];
			char name[80];
			Str255 wtitle;
			extern GopherState g_gopher;

			gopher_build_uri(url, sizeof(url),
			    g_gopher.cur_host, g_gopher.cur_port,
			    g_gopher.cur_type,
			    g_gopher.cur_selector);

			/* Get name from window title
			 * (strip "Geomys - " prefix) */
			GetWTitle(g_window, wtitle);
			{
				short len = wtitle[0];
				char *p;

				if (len >= (short)sizeof(name))
					len = sizeof(name) - 1;
				memcpy(name, wtitle + 1, len);
				name[len] = '\0';
				p = strstr(name, " - ");
				if (p)
					memmove(name, p + 3,
					    strlen(p + 3) + 1);
			}

			/* Fallback to URL if name is empty */
			if (!name[0])
				strncpy(name, url,
				    sizeof(name) - 1);

			favorites_add(&g_prefs, name, url);
			break;
		}
		default:
			favorites_menu_click(&g_prefs, item);
			break;
		}
		break;
#endif
	case OPTIONS_MENU_ID:
		switch (item) {
		case OPT_MENU_HOME:
			do_home_page_dialog();
			break;
		case OPT_MENU_DETAILS:
			g_prefs.show_details =
			    !g_prefs.show_details;
			if (options_menu)
				CheckItem(options_menu,
				    OPT_MENU_DETAILS,
				    g_prefs.show_details != 0);
			prefs_save(&g_prefs);
			if (g_window) {
				GrafPtr save;
				GetPort(&save);
				SetPort(g_window);
				content_draw(g_window);
				SetPort(save);
			}
			break;
		}
		break;
	case FONT_MENU_ID:
		switch (item) {
		case FONT_MONACO9:
			g_prefs.font_id = 4;   /* Monaco */
			g_prefs.font_size = 9;
			break;
		case FONT_MONACO12:
			g_prefs.font_id = 4;   /* Monaco */
			g_prefs.font_size = 12;
			break;
		case FONT_COURIER10:
			g_prefs.font_id = 22;  /* Courier */
			g_prefs.font_size = 10;
			break;
		case FONT_CHICAGO12:
			g_prefs.font_id = 0;   /* Chicago */
			g_prefs.font_size = 12;
			break;
		case FONT_GENEVA9:
			g_prefs.font_id = 3;   /* Geneva */
			g_prefs.font_size = 9;
			break;
		case FONT_GENEVA10:
			g_prefs.font_id = 3;   /* Geneva */
			g_prefs.font_size = 10;
			break;
		}
		/* Update checkmarks */
		if (font_submenu) {
			CheckItem(font_submenu, FONT_MONACO9,
			    item == FONT_MONACO9);
			CheckItem(font_submenu, FONT_MONACO12,
			    item == FONT_MONACO12);
			CheckItem(font_submenu, FONT_COURIER10,
			    item == FONT_COURIER10);
			CheckItem(font_submenu, FONT_CHICAGO12,
			    item == FONT_CHICAGO12);
			CheckItem(font_submenu, FONT_GENEVA9,
			    item == FONT_GENEVA9);
			CheckItem(font_submenu, FONT_GENEVA10,
			    item == FONT_GENEVA10);
		}
		/* Save and redraw */
		prefs_save(&g_prefs);
		content_update_font();
		if (g_window) {
			GrafPtr save;
			GetPort(&save);
			SetPort(g_window);
			content_update_scroll(g_window);
			content_draw(g_window);
			SetPort(save);
		}
		break;
	case STYLE_MENU_ID:
		switch (item) {
		case STYLE_ITEM_TRADITIONAL:
			g_prefs.page_style = STYLE_TRADITIONAL;
			break;
		case STYLE_ITEM_PLAIN:
			g_prefs.page_style = STYLE_PLAIN;
			break;
		case STYLE_ITEM_MARKDOWN:
			g_prefs.page_style = STYLE_MARKDOWN;
			break;
		}
		/* Update checkmarks */
		if (style_submenu) {
			CheckItem(style_submenu, STYLE_ITEM_TRADITIONAL,
			    item == STYLE_ITEM_TRADITIONAL);
			CheckItem(style_submenu, STYLE_ITEM_PLAIN,
			    item == STYLE_ITEM_PLAIN);
			CheckItem(style_submenu, STYLE_ITEM_MARKDOWN,
			    item == STYLE_ITEM_MARKDOWN);
		}
		/* Save and redraw */
		prefs_save(&g_prefs);
		content_update_font();
		if (g_window) {
			GrafPtr save;
			GetPort(&save);
			SetPort(g_window);
			content_update_scroll(g_window);
			content_draw(g_window);
			SetPort(save);
		}
		break;
	default:
		handled = false;
		break;
	}

	HiliteMenu(0);
	return handled;
}
