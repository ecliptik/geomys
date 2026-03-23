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
#include "savefile.h"
#ifdef GEOMYS_THEMES
#include "theme.h"
#include "color.h"
#endif

/* Menu handles (private to this module) */
static MenuHandle apple_menu, file_menu, edit_menu;
static MenuHandle favorites_menu, options_menu;
static MenuHandle window_menu;
static MenuHandle font_submenu;
static MenuHandle style_submenu;
#ifdef GEOMYS_THEMES
static MenuHandle theme_submenu;
#endif

/* main.c globals */
extern GeomysPrefs g_prefs;

#if GEOMYS_MAX_WINDOWS > 1
static void update_window_menu(void);
static void handle_window_menu(short item);
#endif

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

	/* Window menu is in MBAR — loaded atomically by GetNewMBar.
	 * Use GetMenuHandle (like Flynn), not GetMenu+InsertMenu
	 * which corrupts menu bar font state on 68000. */
	window_menu = GetMenuHandle(WINDOW_MENU_ID);
#if GEOMYS_MAX_WINDOWS == 1
	/* Single-window: remove Window menu from bar */
	if (window_menu)
		DeleteMenu(WINDOW_MENU_ID);
	window_menu = 0L;
#endif

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

	/* Load and insert Theme hierarchical submenu */
#ifdef GEOMYS_THEMES
	theme_submenu = GetMenu(THEME_MENU_ID);
	if (theme_submenu)
		InsertMenu(theme_submenu, -1);

	/* Set hierarchical menu marker for Theme in Options */
	if (options_menu) {
		SetItemCmd(options_menu, OPT_MENU_THEME, 0x1B);
		SetItemMark(options_menu, OPT_MENU_THEME,
		    THEME_MENU_ID);
	}

	/* Set initial theme checkmark */
	if (theme_submenu) {
		short t;
		for (t = THEME_ITEM_LIGHT; t <= THEME_ITEM_LAST; t++)
			CheckItem(theme_submenu, t, false);
		/* Map theme_id to menu item (items are 1-indexed,
		 * but separator at item 3 offsets color themes) */
		{
			short check_item;
			if (g_prefs.theme_id <= THEME_DARK)
				check_item = g_prefs.theme_id + 1;
			else
				check_item = g_prefs.theme_id + 2; /* +2 for separator */
			if (check_item >= THEME_ITEM_LIGHT &&
			    check_item <= THEME_ITEM_LAST)
				CheckItem(theme_submenu, check_item, true);
		}

		/* Disable color themes on monochrome systems */
#ifdef GEOMYS_COLOR
		if (!g_has_color_qd)
#endif
		{
			short ci;
			for (ci = THEME_ITEM_SOLARIZED_L; ci <= THEME_ITEM_LAST; ci++)
				DisableItem(theme_submenu, ci);
		}
	}
#endif /* GEOMYS_THEMES */

	/* Set initial Show Details checkmark */
	if (options_menu)
		CheckItem(options_menu, OPT_MENU_DETAILS,
		    g_prefs.show_details != 0);

	/* Set initial Status Bar checkmark */
	if (options_menu)
		CheckItem(options_menu, OPT_MENU_STATUS_BAR,
		    g_prefs.show_status_bar != 0);

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

	/* File menu */
	if (file_menu) {
		/* New Window: disable when at max sessions */
		if (session_count() >= GEOMYS_MAX_WINDOWS)
			DisableItem(file_menu, FILE_MENU_NEW_WIN);
		else
			EnableItem(file_menu, FILE_MENU_NEW_WIN);

#ifdef GEOMYS_DOWNLOAD
		{
			if (g_gopher.page_type != PAGE_NONE &&
			    g_app_state == APP_STATE_IDLE)
				EnableItem(file_menu,
				    FILE_MENU_SAVE_AS);
			else
				DisableItem(file_menu,
				    FILE_MENU_SAVE_AS);
		}
#else
		DisableItem(file_menu, FILE_MENU_SAVE_AS);
#endif
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
			/* DA controls Undo — show "Undo" (HIG p.113) */
			SetMenuItemText(edit_menu, EDIT_MENU_UNDO,
			    "\pUndo");
			EnableItem(edit_menu, EDIT_MENU_UNDO);
			EnableItem(edit_menu, EDIT_MENU_CUT);
			EnableItem(edit_menu, EDIT_MENU_COPY);
			EnableItem(edit_menu, EDIT_MENU_PASTE);
			EnableItem(edit_menu, EDIT_MENU_CLEAR);
			EnableItem(edit_menu, EDIT_MENU_SELALL);
#ifdef GEOMYS_CLIPBOARD
		} else if (browser_get_focus() == FOCUS_ADDR_BAR) {
			/* No undo for address bar — "Can't Undo" per HIG */
			SetMenuItemText(edit_menu, EDIT_MENU_UNDO,
			    "\pCan\325t Undo");
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
			SetMenuItemText(edit_menu, EDIT_MENU_UNDO,
			    "\pCan\325t Undo");
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
			SetMenuItemText(edit_menu, EDIT_MENU_UNDO,
			    "\pCan\325t Undo");
			DisableItem(edit_menu, EDIT_MENU_UNDO);
			DisableItem(edit_menu, EDIT_MENU_CUT);
			DisableItem(edit_menu, EDIT_MENU_COPY);
			DisableItem(edit_menu, EDIT_MENU_PASTE);
			DisableItem(edit_menu, EDIT_MENU_CLEAR);
			DisableItem(edit_menu, EDIT_MENU_SELALL);
		}
	}

#if GEOMYS_MAX_WINDOWS > 1
	update_window_menu();
#endif
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
	case FILE_MENU_NEW_WIN:
		do_new_window();
		break;
	case FILE_MENU_SAVE_AS:
		do_save_page();
		break;
	case FILE_MENU_CLOSE:
		if (active_session)
			session_destroy_and_fixup(active_session);
		else
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

#if GEOMYS_MAX_WINDOWS > 1
/*
 * update_window_menu - Rebuild the Window menu dynamically.
 * Shows "N of M Windows" header, then lists each open window
 * with a checkmark on the active session.
 */
static void
update_window_menu(void)
{
	short i, item_num;
	char label[80];
	Str255 ps;

	if (!window_menu)
		return;

	/* Clear all items after separator (item 2) */
	while (CountMItems(window_menu) > 2)
		DeleteMenuItem(window_menu,
		    CountMItems(window_menu));

	/* Header: "N of M Windows" */
	snprintf(label, sizeof(label), "%d of %d Windows",
	    session_count(), GEOMYS_MAX_WINDOWS);
	c2pstr(ps, label);
	SetMenuItemText(window_menu, WIN_MENU_HEADER, ps);
	DisableItem(window_menu, WIN_MENU_HEADER);

	/* List open windows */
	item_num = WIN_MENU_FIRST_WIN;
	for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
		BrowserSession *s = session_get(i);
		Str255 wtitle;

		if (!s)
			continue;

		/* Get window title */
		if (s->window) {
			GetWTitle(s->window, wtitle);
			if (wtitle[0] == 0) {
				/* Fallback for untitled windows */
				c2pstr(wtitle, "Geomys");
			}
		} else {
			c2pstr(wtitle, "Geomys");
		}

		/* Use AppendMenu + SetMenuItemText to avoid
		 * metacharacter interpretation */
		AppendMenu(window_menu, "\p ");
		SetMenuItemText(window_menu, item_num, wtitle);

		/* Checkmark on active window */
		CheckItem(window_menu, item_num,
		    s == active_session);
		item_num++;
	}
}

/*
 * handle_window_menu - Handle click on a Window menu item.
 * Items 3+ correspond to session windows.
 */
static void
handle_window_menu(short item)
{
	short i, item_num;

	if (item < WIN_MENU_FIRST_WIN)
		return;

	/* Map menu item to session */
	item_num = WIN_MENU_FIRST_WIN;
	for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
		BrowserSession *s = session_get(i);

		if (!s)
			continue;

		if (item_num == item) {
			/* Switch to this session's window */
			if (s->window)
				SelectWindow(s->window);
			return;
		}
		item_num++;
	}
}
#endif /* GEOMYS_MAX_WINDOWS > 1 */

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

			gopher_build_uri(url, sizeof(url),
			    g_gopher.cur_host, g_gopher.cur_port,
			    g_gopher.cur_type,
			    g_gopher.cur_selector);

			/* Get name from window title */
			GetWTitle(g_window, wtitle);
			{
				short len = wtitle[0];

				if (len >= (short)sizeof(name))
					len = sizeof(name) - 1;
				memcpy(name, wtitle + 1, len);
				name[len] = '\0';
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
#if GEOMYS_MAX_WINDOWS > 1
	case WINDOW_MENU_ID:
		handle_window_menu(item);
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
				content_recalc_width(g_window);
				content_draw(g_window);
				SetPort(save);
			}
			break;
		case OPT_MENU_STATUS_BAR:
			g_prefs.show_status_bar =
			    !g_prefs.show_status_bar;
			if (options_menu)
				CheckItem(options_menu,
				    OPT_MENU_STATUS_BAR,
				    g_prefs.show_status_bar != 0);
			prefs_save(&g_prefs);
			/* Resize all windows to reclaim/add status bar space */
			{
				short wi;
				for (wi = 0; wi < GEOMYS_MAX_WINDOWS; wi++) {
					BrowserSession *s = session_get(wi);
					if (s && s->window) {
						GrafPtr save;
						GetPort(&save);
						SetPort(s->window);
						content_resize(s->window);
						InvalRect(&s->window->portRect);
						SetPort(save);
					}
				}
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
			content_recalc_width(g_window);
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
			content_recalc_width(g_window);
			content_update_scroll(g_window);
			content_draw(g_window);
			SetPort(save);
		}
		break;
#ifdef GEOMYS_THEMES
	case THEME_MENU_ID: {
		short new_theme;

		/* Map menu item to theme index (separator at item 3) */
		if (item <= THEME_ITEM_DARK)
			new_theme = item - 1;
		else
			new_theme = item - 2; /* -2 for separator */

		if (new_theme >= 0 && new_theme < THEME_COUNT) {
			g_prefs.theme_id = new_theme;
			theme_set(new_theme);

			/* Update checkmarks */
			if (theme_submenu) {
				short t;
				for (t = THEME_ITEM_LIGHT; t <= THEME_ITEM_LAST; t++)
					CheckItem(theme_submenu, t, t == item);
			}

			/* Save and redraw ALL windows */
			prefs_save(&g_prefs);
			{
				short wi;
				for (wi = 0; wi < GEOMYS_MAX_WINDOWS; wi++) {
					BrowserSession *s = session_get(wi);
					if (s && s->window) {
						GrafPtr save;
						GetPort(&save);
						SetPort(s->window);
						content_mark_all_dirty();
						content_recalc_width(s->window);
						content_update_scroll(s->window);
						content_draw(s->window);
						browser_draw_status(s->window);
						SetPort(save);
					}
				}
			}
		}
		break;
	}
#endif /* GEOMYS_THEMES */
	default:
		handled = false;
		break;
	}

	HiliteMenu(0);
	return handled;
}
