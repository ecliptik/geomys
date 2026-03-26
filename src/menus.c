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
#include "history.h"
#include "gopher_icons.h"
#include "sysutil.h"
#ifdef GEOMYS_PRINT
#include "print.h"
#endif
#ifdef GEOMYS_THEMES
#include "theme.h"
#include "color.h"
#endif
#ifdef GEOMYS_GOPHER_PLUS
#include "gopherplus.h"
#endif

/* Menu handles (private to this module) */
static MenuHandle apple_menu, file_menu, edit_menu;
static MenuHandle go_menu;
static MenuHandle favorites_menu, options_menu;
static MenuHandle window_menu;
static MenuHandle font_submenu;
static MenuHandle size_submenu;
static MenuHandle style_submenu;
#ifdef GEOMYS_THEMES
static MenuHandle theme_submenu;
#endif

/* System 7+ SICN menu icon support.
 * Uses SetItemIcon + SetItemCmd(0x1E) per Inside Macintosh:
 * Toolbox Essentials p.3-101. Only works on menu items
 * WITHOUT keyboard shortcuts (0x1E replaces cmdChar byte). */
static unsigned char g_has_menu_icons = 0;

/* SICN 256 (Folder) maps to icon number 0 (= no icon).
 * Use this duplicate at ID 289 (icon number 33) for menus. */
#define SICN_MENU_FOLDER 289

/* main.c globals and functions */
extern GeomysPrefs g_prefs;
extern void navigate_history_entry(const HistoryEntry *e,
    short direction);
extern void do_refresh(void);

#if GEOMYS_MAX_WINDOWS > 1
static void update_window_menu(void);
static void handle_window_menu(short item);
#endif

unsigned char
menu_has_icons(void)
{
	return g_has_menu_icons;
}

/*
 * menu_set_item_sicn - Attach a SICN icon to a menu item.
 *
 * System 7+ only. Uses the 0x1E command byte convention
 * from Inside Macintosh: Toolbox Essentials (p.3-101).
 * WARNING: This replaces the keyboard equivalent byte,
 * so only use on items WITHOUT Cmd-key shortcuts.
 */
void
menu_set_item_sicn(MenuHandle menu, short item,
    short sicn_id)
{
	short icon_num;

	if (!g_has_menu_icons || !menu || sicn_id == 0)
		return;

	/* SICN 256 (Folder) maps to icon number 0
	 * (= no icon). Use duplicate SICN 289 instead. */
	if (sicn_id == SICN_FOLDER)
		sicn_id = SICN_MENU_FOLDER;

	icon_num = sicn_id - 256;
	if (icon_num < 1 || icon_num > 255)
		return;

	SetItemIcon(menu, item, (Byte)icon_num);
	SetItemCmd(menu, item, 0x1E);
}

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
	go_menu = GetMenuHandle(GO_MENU_ID);
	favorites_menu = GetMenuHandle(FAVORITES_MENU_ID);
	options_menu = GetMenuHandle(OPTIONS_MENU_ID);

	/* Detect System 7+ for SICN menu icon support */
	{
		long sysv;
		if (TrapAvailable(_GestaltDispatch) &&
		    Gestalt(gestaltSystemVersion, &sysv) ==
		    noErr && sysv >= 0x0700)
			g_has_menu_icons = 1;
	}

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
	if (font_submenu) {
		InsertMenu(font_submenu, -1);

		/* Gestalt-gate System 7 fonts */
		if (g_has_menu_icons) {
			AppendMenu(font_submenu, "\p ");
			SetMenuItemText(font_submenu,
			    FONT_HELVETICA, "\pHelvetica");
			AppendMenu(font_submenu, "\p ");
			SetMenuItemText(font_submenu,
			    FONT_TIMES, "\pTimes");
			AppendMenu(font_submenu, "\p ");
			SetMenuItemText(font_submenu,
			    FONT_PALATINO, "\pPalatino");
		}
	}

	/* Load and insert Size hierarchical submenu */
	size_submenu = GetMenu(SIZE_MENU_ID);
	if (size_submenu)
		InsertMenu(size_submenu, -1);

	/* Load and insert Page Style hierarchical submenu */
	style_submenu = GetMenu(STYLE_MENU_ID);
	if (style_submenu)
		InsertMenu(style_submenu, -1);

	/* Set hierarchical menu markers in Options */
	if (options_menu) {
		SetItemCmd(options_menu, OPT_MENU_FONT, 0x1B);
		SetItemMark(options_menu, OPT_MENU_FONT,
		    FONT_MENU_ID);
		SetItemCmd(options_menu, OPT_MENU_SIZE, 0x1B);
		SetItemMark(options_menu, OPT_MENU_SIZE,
		    SIZE_MENU_ID);
		SetItemCmd(options_menu, OPT_MENU_STYLE, 0x1B);
		SetItemMark(options_menu, OPT_MENU_STYLE,
		    STYLE_MENU_ID);
	}

	/* Set initial style checkmark based on prefs */
	if (style_submenu) {
		CheckItem(style_submenu, STYLE_ITEM_TEXT,
		    g_prefs.page_style == STYLE_TEXT);
		CheckItem(style_submenu, STYLE_ITEM_ICONS,
		    g_prefs.page_style == STYLE_ICONS);
	}

	/* Set initial font checkmark based on prefs (font only) */
	if (font_submenu) {
		short fi, font_count;

		font_count = CountMItems(font_submenu);
		for (fi = 1; fi <= font_count; fi++)
			CheckItem(font_submenu, fi, false);

		if (g_prefs.font_id == 4)
			CheckItem(font_submenu, FONT_MONACO, true);
		else if (g_prefs.font_id == 3)
			CheckItem(font_submenu, FONT_GENEVA, true);
		else if (g_prefs.font_id == 0)
			CheckItem(font_submenu, FONT_CHICAGO, true);
		else if (g_prefs.font_id == 22)
			CheckItem(font_submenu, FONT_COURIER, true);
		else if (g_prefs.font_id == 2)
			CheckItem(font_submenu, FONT_NEWYORK, true);
		else if (g_prefs.font_id == 21 && g_has_menu_icons)
			CheckItem(font_submenu,
			    FONT_HELVETICA, true);
		else if (g_prefs.font_id == 20 && g_has_menu_icons)
			CheckItem(font_submenu, FONT_TIMES, true);
		else if (g_prefs.font_id == 16 && g_has_menu_icons)
			CheckItem(font_submenu,
			    FONT_PALATINO, true);
	}

	/* Set initial size checkmark based on prefs (size only) */
	if (size_submenu) {
		CheckItem(size_submenu, SIZE_9,
		    g_prefs.font_size == 9);
		CheckItem(size_submenu, SIZE_10,
		    g_prefs.font_size == 10);
		CheckItem(size_submenu, SIZE_12,
		    g_prefs.font_size == 12);
		CheckItem(size_submenu, SIZE_14,
		    g_prefs.font_size == 14);
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

	/* Set initial Show/Hide Details text */
	if (options_menu)
		SetMenuItemText(options_menu, OPT_MENU_DETAILS,
		    g_prefs.show_details ?
		    "\pHide Details" : "\pShow Details");

	/* Set initial Show/Hide Status Bar text */
	if (options_menu)
		SetMenuItemText(options_menu, OPT_MENU_STATUS_BAR,
		    g_prefs.show_status_bar ?
		    "\pHide Status Bar" :
		    "\pShow Status Bar");

#ifdef GEOMYS_GOPHER_PLUS
	/* Set initial Gopher+ On/Off text */
	if (options_menu)
		SetMenuItemText(options_menu,
		    OPT_MENU_GOPHER_PLUS,
		    g_prefs.gopher_plus ?
		    "\pGopher+ Off" :
		    "\pGopher+ On");
#endif

	/* Favorites menu — managed by favorites.c */

	/* Add SICN icon to Go > Home (System 7+).
	 * Other Go items have Cmd-key shortcuts which
	 * conflict with the 0x1E SICN command byte. */
	menu_set_item_sicn(go_menu, GO_MENU_HOME,
	    SICN_HOME);

	DrawMenuBar();
}

static void update_go_menu_history(void);

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
			DisableItem(file_menu, FILE_MENU_NEW);
		else
			EnableItem(file_menu, FILE_MENU_NEW);

		/* Open: always enabled when a window exists */
		if (g_window)
			EnableItem(file_menu, FILE_MENU_OPEN);
		else
			DisableItem(file_menu, FILE_MENU_OPEN);

#ifdef GEOMYS_GOPHER_PLUS
		/* Get Info: enabled when a directory item is selected */
		if (g_gopher.page_type == PAGE_DIRECTORY &&
		    g_app_state == APP_STATE_IDLE &&
		    content_get_selected_row() >= 0)
			EnableItem(file_menu, FILE_MENU_GETINFO);
		else
			DisableItem(file_menu, FILE_MENU_GETINFO);
#else
		DisableItem(file_menu, FILE_MENU_GETINFO);
#endif

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

#ifdef GEOMYS_PRINT
		EnableItem(file_menu, FILE_MENU_PAGE_SETUP);
		if (g_gopher.page_type != PAGE_NONE &&
		    g_app_state == APP_STATE_IDLE)
			EnableItem(file_menu, FILE_MENU_PRINT);
		else
			DisableItem(file_menu, FILE_MENU_PRINT);
#else
		DisableItem(file_menu, FILE_MENU_PAGE_SETUP);
		DisableItem(file_menu, FILE_MENU_PRINT);
#endif
		if (g_window)
			EnableItem(file_menu, FILE_MENU_CLOSE);
		else
			DisableItem(file_menu, FILE_MENU_CLOSE);
		EnableItem(file_menu, FILE_MENU_QUIT);
	}

	/* Go menu */
	if (go_menu) {
		if (history_can_back())
			EnableItem(go_menu, GO_MENU_BACK);
		else
			DisableItem(go_menu, GO_MENU_BACK);
		if (history_can_forward())
			EnableItem(go_menu, GO_MENU_FORWARD);
		else
			DisableItem(go_menu, GO_MENU_FORWARD);
		if (g_prefs.home_url[0])
			EnableItem(go_menu, GO_MENU_HOME);
		else
			DisableItem(go_menu, GO_MENU_HOME);
		if (g_gopher.cur_host[0] &&
		    g_app_state == APP_STATE_IDLE)
			EnableItem(go_menu, GO_MENU_REFRESH);
		else
			DisableItem(go_menu, GO_MENU_REFRESH);
		if (g_app_state == APP_STATE_LOADING &&
		    g_gopher.receiving)
			EnableItem(go_menu, GO_MENU_STOP);
		else
			DisableItem(go_menu, GO_MENU_STOP);

		update_go_menu_history();
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
			if (browser_can_undo()) {
				SetMenuItemText(edit_menu,
				    EDIT_MENU_UNDO,
				    browser_is_redo() ?
				    "\pRedo" : "\pUndo");
				EnableItem(edit_menu, EDIT_MENU_UNDO);
			} else {
				SetMenuItemText(edit_menu,
				    EDIT_MENU_UNDO,
				    "\pCan\325t Undo");
				DisableItem(edit_menu,
				    EDIT_MENU_UNDO);
			}
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

#ifdef GEOMYS_CLIPBOARD
		/* Find items: enabled when page has content */
		if (!da_active &&
		    g_gopher.page_type != PAGE_NONE) {
			EnableItem(edit_menu, EDIT_MENU_FIND);
			if (content_find_active())
				EnableItem(edit_menu,
				    EDIT_MENU_FIND_AGAIN);
			else
				DisableItem(edit_menu,
				    EDIT_MENU_FIND_AGAIN);
		} else {
			DisableItem(edit_menu, EDIT_MENU_FIND);
			DisableItem(edit_menu,
			    EDIT_MENU_FIND_AGAIN);
		}
#endif

		/* Show/Hide Clipboard toggle (HIG p.75) */
		EnableItem(edit_menu, EDIT_MENU_SHOW_CLIP);
		SetMenuItemText(edit_menu, EDIT_MENU_SHOW_CLIP,
		    clipboard_window_ptr() ?
		    "\pHide Clipboard" :
		    "\pShow Clipboard");
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
	case FILE_MENU_NEW:
		do_new_window();
		break;
	case FILE_MENU_OPEN:
		browser_set_focus(FOCUS_ADDR_BAR);
		browser_activate(true);
#ifdef GEOMYS_CLIPBOARD
		browser_edit_select_all();
#endif
		break;
#ifdef GEOMYS_GOPHER_PLUS
	case FILE_MENU_GETINFO:
		do_getinfo_dialog();
		break;
#endif
	case FILE_MENU_SAVE_AS:
		do_save_page();
		break;
#ifdef GEOMYS_PRINT
	case FILE_MENU_PAGE_SETUP:
		do_page_setup();
		break;
	case FILE_MENU_PRINT:
		do_print();
		break;
#endif
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

/*
 * Clipboard window — HIG p.112: "The Clipboard window looks
 * and acts like a document window. The contents are visible,
 * but not editable."
 *
 * Uses a noGrowDocProc window with a vertical scrollbar.
 * windowKind set to CLIP_WIN_KIND so the main event loop
 * can identify it for update, close, and drag events.
 */
#define CLIP_WIN_KIND   100
#define CLIP_WIN_W      300
#define CLIP_WIN_H      160
#define CLIP_MIN_W      150
#define CLIP_MIN_H      80
#define SCROLLBAR_W     15

static WindowPtr g_clip_window = nil;
static TEHandle g_clip_te = nil;
static ControlHandle g_clip_vscroll = nil;
static ControlHandle g_clip_hscroll = nil;

/* Recalculate TE view/dest rects from window portRect */
static void
clip_calc_rects(Rect *view_r, Rect *dest_r)
{
	*view_r = g_clip_window->portRect;
	view_r->left += 4;
	view_r->top += 4;
	view_r->right -= (SCROLLBAR_W + 4);
	view_r->bottom -= (SCROLLBAR_W + 4);
	*dest_r = *view_r;
	/* Wide dest rect so TE doesn't wrap — hscroll
	 * handles horizontal overflow */
	dest_r->right = dest_r->left + 2000;
}

/* Vertical scroll action proc */
static pascal void
clip_vscroll_action(ControlHandle ctl, short part)
{
	short delta = 0, old_val, new_val, max_val;
	short page_lines;

	if (!g_clip_te || part == 0)
		return;

	page_lines = ((*g_clip_te)->viewRect.bottom -
	    (*g_clip_te)->viewRect.top) /
	    (*g_clip_te)->lineHeight;
	if (page_lines < 1) page_lines = 1;

	switch (part) {
	case inUpButton:    delta = -1; break;
	case inDownButton:  delta = 1; break;
	case inPageUp:      delta = -(page_lines - 1); break;
	case inPageDown:    delta = page_lines - 1; break;
	}

	old_val = GetControlValue(ctl);
	max_val = GetControlMaximum(ctl);
	new_val = old_val + delta;
	if (new_val < 0) new_val = 0;
	if (new_val > max_val) new_val = max_val;
	SetControlValue(ctl, new_val);

	TEScroll(0, (old_val - new_val) *
	    (*g_clip_te)->lineHeight, g_clip_te);
}

/* Horizontal scroll action proc */
static pascal void
clip_hscroll_action(ControlHandle ctl, short part)
{
	short delta = 0, old_val, new_val, max_val;
	short page_w;

	if (!g_clip_te || part == 0)
		return;

	page_w = (*g_clip_te)->viewRect.right -
	    (*g_clip_te)->viewRect.left;

	switch (part) {
	case inUpButton:    delta = -8; break;
	case inDownButton:  delta = 8; break;
	case inPageUp:      delta = -(page_w - 16); break;
	case inPageDown:    delta = page_w - 16; break;
	}

	old_val = GetControlValue(ctl);
	max_val = GetControlMaximum(ctl);
	new_val = old_val + delta;
	if (new_val < 0) new_val = 0;
	if (new_val > max_val) new_val = max_val;
	SetControlValue(ctl, new_val);

	TEScroll(old_val - new_val, 0, g_clip_te);
}

/* Update both scrollbar ranges from TE state */
static void
clip_update_scroll(void)
{
	short n_lines, vis_lines, max_v;
	short max_w, vis_w, max_h;

	if (!g_clip_te)
		return;

	/* Vertical */
	if (g_clip_vscroll) {
		n_lines = (*g_clip_te)->nLines;
		if ((*g_clip_te)->teLength > 0 &&
		    (*((char **)(*g_clip_te)->hText))
		    [(*g_clip_te)->teLength - 1] != '\r')
			n_lines++;

		vis_lines = ((*g_clip_te)->viewRect.bottom -
		    (*g_clip_te)->viewRect.top) /
		    (*g_clip_te)->lineHeight;

		max_v = n_lines - vis_lines;
		if (max_v < 0) max_v = 0;
		SetControlMaximum(g_clip_vscroll, max_v);
	}

	/* Horizontal — find widest line */
	if (g_clip_hscroll) {
		short i;
		GrafPtr save;

		GetPort(&save);
		SetPort(g_clip_window);
		TextFont(3);
		TextSize(9);

		max_w = 0;
		for (i = 0; i < (*g_clip_te)->nLines; i++) {
			short start, end, w;
			char *text;

			start = (*g_clip_te)->lineStarts[i];
			if (i + 1 < (*g_clip_te)->nLines)
				end = (*g_clip_te)->lineStarts[i + 1];
			else
				end = (*g_clip_te)->teLength;

			HLock((*g_clip_te)->hText);
			text = *(*g_clip_te)->hText;
			w = TextWidth(text, start, end - start);
			HUnlock((*g_clip_te)->hText);

			if (w > max_w) max_w = w;
		}

		vis_w = (*g_clip_te)->viewRect.right -
		    (*g_clip_te)->viewRect.left;
		max_h = max_w - vis_w;
		if (max_h < 0) max_h = 0;
		SetControlMaximum(g_clip_hscroll, max_h);

		SetPort(save);
	}
}

/* Reposition scrollbars and TE after window resize */
static void
clip_resize(void)
{
	Rect r, view_r, dest_r;
	short dh, dv;

	if (!g_clip_window)
		return;

	r = g_clip_window->portRect;

	/* Vertical scrollbar: right edge */
	if (g_clip_vscroll) {
		MoveControl(g_clip_vscroll,
		    r.right - SCROLLBAR_W, r.top - 1);
		SizeControl(g_clip_vscroll,
		    SCROLLBAR_W + 1,
		    r.bottom - r.top - (SCROLLBAR_W - 2));
	}

	/* Horizontal scrollbar: bottom edge */
	if (g_clip_hscroll) {
		MoveControl(g_clip_hscroll,
		    r.left - 1, r.bottom - SCROLLBAR_W);
		SizeControl(g_clip_hscroll,
		    r.right - r.left - (SCROLLBAR_W - 2),
		    SCROLLBAR_W + 1);
	}

	/* Update TE rects */
	if (g_clip_te) {
		clip_calc_rects(&view_r, &dest_r);

		/* Adjust dest_r.top to maintain scroll pos */
		dv = (*g_clip_te)->viewRect.top -
		    (*g_clip_te)->destRect.top;
		dh = (*g_clip_te)->viewRect.left -
		    (*g_clip_te)->destRect.left;
		dest_r.top = view_r.top - dv;
		dest_r.left = view_r.left - dh;

		(*g_clip_te)->viewRect = view_r;
		(*g_clip_te)->destRect = dest_r;
		TECalText(g_clip_te);

		clip_update_scroll();
	}
}

static void
do_show_clipboard(void)
{
	Handle scrap_h;
	long scrap_len, scrap_offset;
	Rect bounds, view_r, dest_r, scroll_r;
	short mbar_h;
	GrafPtr save;

	/* If already open, just bring to front */
	if (g_clip_window) {
		SelectWindow(g_clip_window);
		return;
	}

	GetPort(&save);

	/* Position window offset from center */
	mbar_h = GetMBarHeight();
	bounds.left = (qd.screenBits.bounds.right -
	    CLIP_WIN_W) / 2;
	bounds.top = mbar_h + 30;
	bounds.right = bounds.left + CLIP_WIN_W;
	bounds.bottom = bounds.top + CLIP_WIN_H;

	/* documentProc (0) = close box + grow box */
	g_clip_window = NewWindow(nil, &bounds,
	    "\pClipboard", true, documentProc,
	    (WindowPtr)-1L, true, 0L);
	if (!g_clip_window) {
		SetPort(save);
		return;
	}

	/* Mark with custom windowKind so event loop
	 * can identify this window */
	((WindowPeek)g_clip_window)->windowKind =
	    CLIP_WIN_KIND;

	SetPort(g_clip_window);

	/* Vertical scrollbar (right edge, above grow box) */
	scroll_r.left = g_clip_window->portRect.right -
	    SCROLLBAR_W;
	scroll_r.top = g_clip_window->portRect.top - 1;
	scroll_r.right = g_clip_window->portRect.right + 1;
	scroll_r.bottom = g_clip_window->portRect.bottom -
	    (SCROLLBAR_W - 1);
	g_clip_vscroll = NewControl(g_clip_window,
	    &scroll_r, "\p", true, 0, 0, 0,
	    scrollBarProc, 0L);

	/* Horizontal scrollbar (bottom edge, left of
	 * grow box) */
	scroll_r.left = g_clip_window->portRect.left - 1;
	scroll_r.top = g_clip_window->portRect.bottom -
	    SCROLLBAR_W;
	scroll_r.right = g_clip_window->portRect.right -
	    (SCROLLBAR_W - 1);
	scroll_r.bottom = g_clip_window->portRect.bottom + 1;
	g_clip_hscroll = NewControl(g_clip_window,
	    &scroll_r, "\p", true, 0, 0, 0,
	    scrollBarProc, 0L);

	/* TextEdit rects */
	TextFont(3);    /* Geneva */
	TextSize(9);
	clip_calc_rects(&view_r, &dest_r);

	g_clip_te = TENew(&dest_r, &view_r);
	if (!g_clip_te) {
		DisposeWindow(g_clip_window);
		g_clip_window = nil;
		g_clip_vscroll = nil;
		g_clip_hscroll = nil;
		SetPort(save);
		return;
	}

	/* Get clipboard text */
	scrap_len = GetScrap(nil, 'TEXT', &scrap_offset);
	if (scrap_len > 0) {
		if (scrap_len > 4096)
			scrap_len = 4096;
		scrap_h = NewHandle(scrap_len);
		if (scrap_h) {
			GetScrap(scrap_h, 'TEXT', &scrap_offset);
			HLock(scrap_h);
			TESetText(*scrap_h, scrap_len, g_clip_te);
			HUnlock(scrap_h);
			DisposeHandle(scrap_h);
		}
	} else {
		TESetText("(Clipboard is empty.)", 21,
		    g_clip_te);
	}

	clip_update_scroll();

	SetPort(save);
}

/*
 * clipboard_window_ptr - Return the clipboard window
 * pointer so the main event loop can check for it.
 */
WindowPtr
clipboard_window_ptr(void)
{
	return g_clip_window;
}

/*
 * clipboard_window_update - Draw the clipboard window
 * content. Called from handle_update in main.c.
 */
void
clipboard_window_update(WindowPtr win)
{
	GrafPtr save;
	Rect grow_r;

	if (win != g_clip_window || !g_clip_te)
		return;

	GetPort(&save);
	SetPort(win);
	EraseRect(&win->portRect);
	TEUpdate(&win->portRect, g_clip_te);
	DrawControls(win);

	/* Draw grow box */
	grow_r.right = win->portRect.right;
	grow_r.bottom = win->portRect.bottom;
	grow_r.left = grow_r.right - SCROLLBAR_W;
	grow_r.top = grow_r.bottom - SCROLLBAR_W;
	DrawGrowIcon(win);

	SetPort(save);
}

/*
 * clipboard_window_close - Close and dispose the
 * clipboard window. Called from inGoAway in main.c.
 */
void
clipboard_window_close(void)
{
	if (g_clip_te) {
		TEDispose(g_clip_te);
		g_clip_te = nil;
	}
	g_clip_vscroll = nil;  /* disposed with window */
	g_clip_hscroll = nil;
	if (g_clip_window) {
		DisposeWindow(g_clip_window);
		g_clip_window = nil;
	}
}

/*
 * clipboard_window_click - Handle a click in the
 * clipboard window content area.
 */
void
clipboard_window_click(WindowPtr win, Point where)
{
	ControlHandle ctl;
	short ctl_part;

	if (win != g_clip_window)
		return;

	SetPort(win);
	GlobalToLocal(&where);
	ctl_part = FindControl(where, win, &ctl);

	if (ctl && ctl_part != 0) {
		if (ctl_part == inThumb) {
			short old_val = GetControlValue(ctl);

			TrackControl(ctl, where, nil);
			{
				short new_val =
				    GetControlValue(ctl);
				if (ctl == g_clip_vscroll)
					TEScroll(0,
					    (old_val - new_val) *
					    (*g_clip_te)->lineHeight,
					    g_clip_te);
				else if (ctl == g_clip_hscroll)
					TEScroll(
					    old_val - new_val, 0,
					    g_clip_te);
			}
		} else if (ctl == g_clip_vscroll) {
			TrackControl(ctl, where,
			    (ControlActionUPP)
			    clip_vscroll_action);
		} else if (ctl == g_clip_hscroll) {
			TrackControl(ctl, where,
			    (ControlActionUPP)
			    clip_hscroll_action);
		}
	}
}

/*
 * clipboard_window_grow - Handle grow box drag.
 * Called from inGrow in main.c.
 */
void
clipboard_window_grow(WindowPtr win, Point where)
{
	Rect limit;
	long new_size;

	if (win != g_clip_window)
		return;

	SetRect(&limit, CLIP_MIN_W, CLIP_MIN_H,
	    qd.screenBits.bounds.right,
	    qd.screenBits.bounds.bottom);
	new_size = GrowWindow(win, where, &limit);
	if (new_size == 0)
		return;

	SetPort(win);
	EraseRect(&win->portRect);
	SizeWindow(win, LoWord(new_size),
	    HiWord(new_size), true);
	clip_resize();
	InvalRect(&win->portRect);
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
		case EDIT_MENU_UNDO:
			browser_edit_undo();
			break;
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

	/* Find commands work regardless of focus */
	switch (item) {
	case EDIT_MENU_FIND:
		do_find_dialog();
		break;
	case EDIT_MENU_FIND_AGAIN:
		content_find_again();
		break;
	}
#endif

	/* Show/Hide Clipboard toggle */
	if (item == EDIT_MENU_SHOW_CLIP) {
		if (g_clip_window)
			clipboard_window_close();
		else
			do_show_clipboard();
	}
}

/* First history item number in Go menu (0 = no history shown) */
static short g_go_history_start = 0;

/*
 * update_go_menu_history - Append history entries to Go menu.
 * Called from update_menus after static items are updated.
 */
static void
update_go_menu_history(void)
{
	short hcount, cur_pos, i, item_num, shown;
	Str255 ps;

	g_go_history_start = 0;

	if (!go_menu)
		return;

	/* Remove any previously appended history items
	 * (items after GO_MENU_STOP = 6) */
	while (CountMItems(go_menu) > GO_MENU_STOP)
		DeleteMenuItem(go_menu,
		    CountMItems(go_menu));

	hcount = history_count();
	cur_pos = history_current_index();

	if (hcount <= 0)
		return;

	/* Separator + "History" heading + entries */
	AppendMenu(go_menu, "\p(-");
	item_num = GO_MENU_STOP + 2;
	AppendMenu(go_menu, "\pHistory");
	DisableItem(go_menu, item_num);
	item_num++;
	g_go_history_start = item_num;

	/* Most recent first, max 10 */
	shown = 0;
	for (i = hcount - 1; i >= 0 && shown < 10;
	    i--, shown++) {
		const HistoryEntry *he;
		char hlabel[80];

		he = history_get(i);
		if (!he)
			continue;

		if (he->title[0])
			strncpy(hlabel, he->title,
			    sizeof(hlabel) - 1);
		else
			strncpy(hlabel, he->host,
			    sizeof(hlabel) - 1);
		hlabel[sizeof(hlabel) - 1] = '\0';

		c2pstr(ps, hlabel);
		AppendMenu(go_menu, "\p ");
		SetMenuItemText(go_menu, item_num, ps);
		CheckItem(go_menu, item_num,
		    i == cur_pos);
		item_num++;
	}
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
	case GO_MENU_ID:
		switch (item) {
		case GO_MENU_BACK:
			if (history_can_back()) {
				history_set_scroll(
				    content_get_scroll_pos());
				navigate_history_entry(
				    history_back(), -1);
			}
			break;
		case GO_MENU_FORWARD:
			if (history_can_forward()) {
				history_set_scroll(
				    content_get_scroll_pos());
				navigate_history_entry(
				    history_forward(), 1);
			}
			break;
		case GO_MENU_HOME:
			if (g_prefs.home_url[0])
				do_navigate_url(g_prefs.home_url);
			break;
		case GO_MENU_REFRESH:
			do_refresh();
			break;
		case GO_MENU_STOP:
			do_cancel_loading();
			break;
		default:
			/* History list items */
			if (g_go_history_start > 0 &&
			    item >= g_go_history_start) {
				short hcount = history_count();
				short offset = item -
				    g_go_history_start;
				short idx = hcount - 1 - offset;

				if (idx >= 0 && idx < hcount)
					navigate_history_to(idx);
			}
			break;
		}
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
		case OPT_MENU_DNS:
			do_dns_server_dialog();
			break;
		case OPT_MENU_DETAILS:
			g_prefs.show_details =
			    !g_prefs.show_details;
			if (options_menu)
				SetMenuItemText(options_menu,
				    OPT_MENU_DETAILS,
				    g_prefs.show_details ?
				    "\pHide Details" :
				    "\pShow Details");
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
				SetMenuItemText(options_menu,
				    OPT_MENU_STATUS_BAR,
				    g_prefs.show_status_bar ?
				    "\pHide Status Bar" :
				    "\pShow Status Bar");
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
#ifdef GEOMYS_GOPHER_PLUS
		case OPT_MENU_GOPHER_PLUS:
			g_prefs.gopher_plus =
			    !g_prefs.gopher_plus;
			if (options_menu)
				SetMenuItemText(options_menu,
				    OPT_MENU_GOPHER_PLUS,
				    g_prefs.gopher_plus ?
				    "\pGopher+ Off" :
				    "\pGopher+ On");
			prefs_save(&g_prefs);
			/* Redraw to update +/score display */
			if (g_window) {
				GrafPtr save;
				GetPort(&save);
				SetPort(g_window);
				content_draw(g_window);
				SetPort(save);
			}
			break;
#endif
		}
		break;
	case FONT_MENU_ID:
		switch (item) {
		case FONT_MONACO:
			g_prefs.font_id = 4;   /* Monaco */
			break;
		case FONT_GENEVA:
			g_prefs.font_id = 3;   /* Geneva */
			break;
		case FONT_CHICAGO:
			g_prefs.font_id = 0;   /* Chicago */
			break;
		case FONT_COURIER:
			g_prefs.font_id = 22;  /* Courier */
			break;
		case FONT_NEWYORK:
			g_prefs.font_id = 2;   /* New York */
			break;
		case FONT_HELVETICA:
			g_prefs.font_id = 21;  /* Helvetica */
			break;
		case FONT_TIMES:
			g_prefs.font_id = 20;  /* Times */
			break;
		case FONT_PALATINO:
			g_prefs.font_id = 16;  /* Palatino */
			break;
		}
		/* Update checkmarks */
		if (font_submenu) {
			short fi, font_count;

			font_count = CountMItems(font_submenu);
			for (fi = 1; fi <= font_count; fi++)
				CheckItem(font_submenu, fi,
				    fi == item);
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
	case SIZE_MENU_ID:
		switch (item) {
		case SIZE_9:
			g_prefs.font_size = 9;
			break;
		case SIZE_10:
			g_prefs.font_size = 10;
			break;
		case SIZE_12:
			g_prefs.font_size = 12;
			break;
		case SIZE_14:
			g_prefs.font_size = 14;
			break;
		}
		/* Update checkmarks */
		if (size_submenu) {
			CheckItem(size_submenu, SIZE_9,
			    item == SIZE_9);
			CheckItem(size_submenu, SIZE_10,
			    item == SIZE_10);
			CheckItem(size_submenu, SIZE_12,
			    item == SIZE_12);
			CheckItem(size_submenu, SIZE_14,
			    item == SIZE_14);
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
		case STYLE_ITEM_TEXT:
			g_prefs.page_style = STYLE_TEXT;
			break;
		case STYLE_ITEM_ICONS:
			g_prefs.page_style = STYLE_ICONS;
			break;
		}
		/* Update checkmarks */
		if (style_submenu) {
			CheckItem(style_submenu, STYLE_ITEM_TEXT,
			    item == STYLE_ITEM_TEXT);
			CheckItem(style_submenu, STYLE_ITEM_ICONS,
			    item == STYLE_ITEM_ICONS);
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

			/* Save and redraw ALL windows.
			 * Erase content to new bg color first so
			 * the background flips in one frame, then
			 * content renders on top. */
			prefs_save(&g_prefs);
			{
				short wi;
				for (wi = 0; wi < GEOMYS_MAX_WINDOWS; wi++) {
					BrowserSession *s = session_get(wi);
					if (s && s->window) {
						GrafPtr save;
						Rect cr;

						GetPort(&save);
						SetPort(s->window);

						/* Instant bg flip */
						content_get_rect(s->window, &cr);
						theme_set_bg(&theme_current()->bg);
						EraseRect(&cr);

						content_mark_all_dirty();
						content_recalc_width(s->window);
						content_update_scroll(s->window);
						content_draw(s->window);
						browser_draw(s->window);
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
