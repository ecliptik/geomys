/*
 * favorites.c - Favorites management for Geomys
 */

#include <Quickdraw.h>
#include <Menus.h>
#include <Dialogs.h>
#include <Windows.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <Multiverse.h>
#include <string.h>
#include <stdio.h>

#include "favorites.h"
#include "settings.h"
#include "dialogs.h"
#include "macutil.h"
#include "main.h"
#include "menus.h"
#include "gopher.h"
#include "gopher_icons.h"

/* Favorites menu: static items then dynamic entries */
#define FAV_STATIC_ITEMS  2   /* Manage + Add */
#define FAV_FIRST_ENTRY   4   /* after separator */
#define FAV_MENU_MAX      5   /* show top 5 in menu */

void
favorites_init_menu(GeomysPrefs *prefs)
{
	favorites_rebuild_menu(prefs);
}

void
favorites_rebuild_menu(GeomysPrefs *prefs)
{
	MenuHandle fav_menu;
	short count, i;
	Str255 item_str;
	short nlen;

	fav_menu = GetMenuHandle(FAVORITES_MENU_ID);
	if (!fav_menu)
		return;

	/* Remove all dynamic items */
	count = CountMItems(fav_menu);
	while (count > FAV_STATIC_ITEMS) {
		DeleteMenuItem(fav_menu, count);
		count--;
	}

	/* Enable Manage, enable/disable Add based on count */
	EnableItem(fav_menu, 1);  /* Manage */
	if (prefs->favorite_count < MAX_FAVORITES)
		EnableItem(fav_menu, 2);  /* Add */
	else
		DisableItem(fav_menu, 2);

	/* Add separator + top 5 entries */
	if (prefs->favorite_count > 0) {
		short show_count = prefs->favorite_count;
		if (show_count > FAV_MENU_MAX)
			show_count = FAV_MENU_MAX;

		AppendMenu(fav_menu, "\p(-");

		for (i = 0; i < show_count; i++) {
			nlen = strlen(prefs->favorites[i].name);
			if (nlen > 254) nlen = 254;
			item_str[0] = nlen;
			memcpy(item_str + 1,
			    prefs->favorites[i].name, nlen);
			AppendMenu(fav_menu, "\p ");
			SetMenuItemText(fav_menu,
			    CountMItems(fav_menu), item_str);

			/* Add type-based SICN icon
			 * (System 7+) */
			if (menu_has_icons()) {
				char fh[64];
				short fp;
				char ft;
				char fs[128];

				if (gopher_parse_uri(
				    prefs->favorites[i].url,
				    fh, sizeof(fh),
				    &fp, &ft,
				    fs, sizeof(fs))) {
					short sid;
					sid =
					    gopher_sicn_for_type(
					    ft);
					if (sid)
						menu_set_item_sicn(
						    fav_menu,
						    CountMItems(
						    fav_menu),
						    sid);
				}
			}
		}
	}
}

void
favorites_add(GeomysPrefs *prefs, const char *name,
    const char *url)
{
	DialogPtr dlg;
	short item;
	short item_type;
	Handle item_h;
	Rect item_rect;
	Str255 pstr;

	if (prefs->favorite_count >= MAX_FAVORITES) {
		ParamText(
		    "\pFavorites list is full (maximum 20).",
		    "\p", "\p", "\p");
		NoteAlert(128, 0L);
		return;
	}

	dlg = get_modal_dialog(DLOG_EDIT_FAV_ID);
	if (!dlg)
		return;
	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	/* Pre-fill name */
	c2pstr(pstr, name);
	GetDialogItem(dlg, 4, &item_type, &item_h, &item_rect);
	SetDialogItemText(item_h, pstr);

	/* Pre-fill URL */
	c2pstr(pstr, url);
	GetDialogItem(dlg, 6, &item_type, &item_h, &item_rect);
	SetDialogItemText(item_h, pstr);

	setup_default_button_outline(dlg, 7);
	SelectDialogItemText(dlg, 4, 0, 32767);

	do {
		ModalDialog((ModalFilterUPP)std_dlg_filter, &item);
	} while (item != 1 && item != 2);

	if (item == 1) {
		GopherFavorite *fav;
		short len;
		char test_name[64], test_url[256];

		/* Validate fields */
		GetDialogItem(dlg, 4, &item_type, &item_h,
		    &item_rect);
		GetDialogItemText(item_h, pstr);
		len = pstr[0];
		if (len >= (short)sizeof(test_name))
			len = sizeof(test_name) - 1;
		memcpy(test_name, pstr + 1, len);
		test_name[len] = '\0';

		GetDialogItem(dlg, 6, &item_type, &item_h,
		    &item_rect);
		GetDialogItemText(item_h, pstr);
		len = pstr[0];
		if (len >= (short)sizeof(test_url))
			len = sizeof(test_url) - 1;
		memcpy(test_url, pstr + 1, len);
		test_url[len] = '\0';

		if (test_name[0] == '\0' || test_url[0] == '\0') {
			DisposeDialog(dlg);
			ParamText(
			    "\pName and URL are required.",
			    "\p", "\p", "\p");
			NoteAlert(128, 0L);
			return;
		}

		fav = &prefs->favorites[prefs->favorite_count];
		strncpy(fav->name, test_name,
		    sizeof(fav->name) - 1);
		fav->name[sizeof(fav->name) - 1] = '\0';
		strncpy(fav->url, test_url,
		    sizeof(fav->url) - 1);
		fav->url[sizeof(fav->url) - 1] = '\0';

		prefs->favorite_count++;
		prefs_save(prefs);
		favorites_rebuild_menu(prefs);
	}

	DisposeDialog(dlg);
}

/* Static list handle for user item draw proc access */
static ListHandle g_fav_list;

/*
 * fav_list_draw_proc - UserItem draw proc for the List Manager
 * list. Called by the Dialog Manager on update events.
 */
static pascal void
fav_list_draw_proc(DialogPtr dlg, short item)
{
	Rect frame_r;
	short item_type;
	Handle item_h;

	(void)item;
	GetDialogItem(dlg, 5, &item_type, &item_h,
	    &frame_r);

	if (g_fav_list) {
		LUpdate(((WindowPtr)dlg)->visRgn,
		    g_fav_list);
	}

	/* Frame around the entire list area */
	FrameRect(&frame_r);
}

/*
 * fav_list_populate - Fill list cells from prefs data.
 * Clears existing rows and adds fresh ones.
 */
static void
fav_list_populate(GeomysPrefs *prefs)
{
	short i, existing;
	Cell cell;

	if (!g_fav_list)
		return;

	/* Remove all existing rows */
	existing = (**g_fav_list).dataBounds.bottom;
	if (existing > 0)
		LDelRow(existing, 0, g_fav_list);

	/* Add rows for each favorite */
	if (prefs->favorite_count > 0) {
		LAddRow(prefs->favorite_count, 0,
		    g_fav_list);
		for (i = 0; i < prefs->favorite_count; i++) {
			cell.h = 0;
			cell.v = i;
			LSetCell(prefs->favorites[i].name,
			    strlen(prefs->favorites[i].name),
			    cell, g_fav_list);
		}
	}
}

/*
 * fav_list_get_selection - Get the selected row index.
 * Returns -1 if nothing is selected.
 */
static short
fav_list_get_selection(void)
{
	Cell cell;

	if (!g_fav_list)
		return -1;

	cell.h = 0;
	cell.v = 0;
	if (LGetSelect(true, &cell, g_fav_list))
		return cell.v;
	return -1;
}

/*
 * fav_list_set_selection - Select a specific row and
 * auto-scroll to make it visible.
 */
static void
fav_list_set_selection(short row)
{
	Cell cell;
	short total;

	if (!g_fav_list)
		return;

	/* Deselect all */
	total = (**g_fav_list).dataBounds.bottom;
	for (cell.v = 0; cell.v < total; cell.v++) {
		cell.h = 0;
		LSetSelect(false, cell, g_fav_list);
	}

	/* Select the requested row */
	if (row >= 0 && row < total) {
		cell.h = 0;
		cell.v = row;
		LSetSelect(true, cell, g_fav_list);
		LAutoScroll(g_fav_list);
	}
}

void
favorites_manage(GeomysPrefs *prefs)
{
	DialogPtr dlg;
	short item;
	short selection = -1;
	short item_type;
	Handle item_h;
	Rect list_rect, rview;
	Rect data_bounds;
	Point cell_size;
	GrafPtr save;
	Boolean done = false;

	dlg = get_modal_dialog(DLOG_FAVORITES_ID);
	if (!dlg)
		return;
	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	setup_default_button_outline(dlg, 6);

	/* Get list area rect from dialog item 5 */
	GetDialogItem(dlg, 5, &item_type, &item_h,
	    &list_rect);

	/* Register user item draw proc for list updates */
	SetDialogItem(dlg, 5, item_type,
	    (Handle)fav_list_draw_proc, &list_rect);

	/* Create List Manager list.
	 * rView excludes scrollbar (15px) and 1px frame. */
	rview = list_rect;
	InsetRect(&rview, 1, 1);
	rview.right -= 15;

	SetRect(&data_bounds, 0, 0, 1,
	    prefs->favorite_count);
	cell_size.h = rview.right - rview.left;
	cell_size.v = 16;

	GetPort(&save);
	SetPort((WindowPtr)dlg);
	g_fav_list = LNew(&rview, &data_bounds,
	    cell_size, 0, (WindowPtr)dlg,
	    true, false, false, true);
	SetPort(save);

	if (!g_fav_list) {
		DisposeDialog(dlg);
		return;
	}

	/* Only allow single selection */
	(**g_fav_list).selFlags = lOnlyOne;

	/* Populate cells from prefs */
	fav_list_populate(prefs);

	while (!done) {
		ModalDialog((ModalFilterUPP)std_dlg_filter,
		    &item);

		switch (item) {
		case 1:  /* Done */
			done = true;
			break;
		case 2:  /* Add */
			if (prefs->favorite_count <
			    MAX_FAVORITES) {
				DialogPtr edlg;
				short eitem;
				short eit;
				Handle eih;
				Rect eir;
				Str255 eps;

				edlg = get_modal_dialog(
				    DLOG_EDIT_FAV_ID);
				if (edlg) {
					center_dialog_on_screen(
					    edlg);
					setup_default_button_outline(
					    edlg, 7);
					SelectDialogItemText(
					    edlg, 4, 0, 0);

					do {
						ModalDialog(
						    (ModalFilterUPP)
						    std_dlg_filter,
						    &eitem);
					} while (eitem != 1 &&
					    eitem != 2);

					if (eitem == 1) {
						GopherFavorite *fav;
						short len;

						fav = &prefs->favorites[
						    prefs->
						    favorite_count];

						GetDialogItem(edlg, 4,
						    &eit, &eih, &eir);
						GetDialogItemText(
						    eih, eps);
						len = eps[0];
						if (len >= 63)
							len = 63;
						memcpy(fav->name,
						    eps + 1, len);
						fav->name[len] = '\0';

						GetDialogItem(edlg, 6,
						    &eit, &eih, &eir);
						GetDialogItemText(
						    eih, eps);
						len = eps[0];
						if (len >= 255)
							len = 255;
						memcpy(fav->url,
						    eps + 1, len);
						fav->url[len] = '\0';

						if (fav->name[0] &&
						    fav->url[0]) {
							prefs->
							    favorite_count++;
							prefs_save(prefs);
							selection =
							    prefs->
							    favorite_count
							    - 1;
						}
					}
					DisposeDialog(edlg);

					/* Rebuild list */
					fav_list_populate(prefs);
					if (selection >= 0)
						fav_list_set_selection(
						    selection);
				}
			}
			break;
		case 3:  /* Edit */
			selection =
			    fav_list_get_selection();
			if (selection >= 0 &&
			    selection <
			    prefs->favorite_count) {
				DialogPtr edlg;
				short eitem;
				short eit;
				Handle eih;
				Rect eir;
				Str255 eps;

				edlg = get_modal_dialog(
				    DLOG_EDIT_FAV_ID);
				if (edlg) {
					center_dialog_on_screen(
					    edlg);
					c2pstr(eps,
					    prefs->favorites[
					    selection].name);
					GetDialogItem(edlg, 4,
					    &eit, &eih, &eir);
					SetDialogItemText(
					    eih, eps);

					c2pstr(eps,
					    prefs->favorites[
					    selection].url);
					GetDialogItem(edlg, 6,
					    &eit, &eih, &eir);
					SetDialogItemText(
					    eih, eps);

					setup_default_button_outline(
					    edlg, 7);
					SelectDialogItemText(
					    edlg, 4, 0, 32767);

					do {
						ModalDialog(
						    (ModalFilterUPP)
						    std_dlg_filter,
						    &eitem);
					} while (eitem != 1 &&
					    eitem != 2);

					if (eitem == 1) {
						GopherFavorite *fav;
						short len;
						Cell cell;

						fav = &prefs->favorites[
						    selection];

						GetDialogItem(edlg, 4,
						    &eit, &eih, &eir);
						GetDialogItemText(
						    eih, eps);
						len = eps[0];
						if (len >= 63)
							len = 63;
						memcpy(fav->name,
						    eps + 1, len);
						fav->name[len] = '\0';

						GetDialogItem(edlg, 6,
						    &eit, &eih, &eir);
						GetDialogItemText(
						    eih, eps);
						len = eps[0];
						if (len >= 255)
							len = 255;
						memcpy(fav->url,
						    eps + 1, len);
						fav->url[len] = '\0';

						prefs_save(prefs);

						/* Update cell text */
						cell.h = 0;
						cell.v = selection;
						LSetCell(fav->name,
						    strlen(fav->name),
						    cell, g_fav_list);
						LDraw(cell,
						    g_fav_list);
					}
					DisposeDialog(edlg);
				}
			}
			break;
		case 5:  /* List click */
		{
			Point mouse;
			Boolean dbl_click;

			GetPort(&save);
			SetPort((WindowPtr)dlg);
			GetMouse(&mouse);
			dbl_click = LClick(mouse, 0,
			    g_fav_list);
			SetPort(save);

			selection =
			    fav_list_get_selection();

			/* Double-click = Go To */
			if (dbl_click && selection >= 0 &&
			    selection <
			    prefs->favorite_count) {
				LDispose(g_fav_list);
				g_fav_list = 0L;
				DisposeDialog(dlg);
				favorites_rebuild_menu(prefs);
				do_navigate_url(
				    prefs->favorites[
				    selection].url);
				return;
			}
			break;
		}
		case 7:  /* Remove */
			selection =
			    fav_list_get_selection();
			if (selection >= 0 &&
			    selection <
			    prefs->favorite_count) {
				short i;

				ParamText(
				    "\pRemove this favorite?",
				    "\p", "\p", "\p");
				if (CautionAlert(129, 0L) == 2) {
					for (i = selection;
					    i < prefs->
					    favorite_count - 1;
					    i++)
						prefs->favorites[i] =
						    prefs->favorites[
						    i + 1];
					prefs->favorite_count--;
					prefs_save(prefs);

					/* Rebuild and reselect */
					fav_list_populate(prefs);
					if (selection >=
					    prefs->favorite_count)
						selection =
						    prefs->
						    favorite_count - 1;
					if (selection >= 0)
						fav_list_set_selection(
						    selection);
				}
			}
			break;
		case 8:  /* Move Up */
			selection =
			    fav_list_get_selection();
			if (selection > 0 &&
			    selection <
			    prefs->favorite_count) {
				GopherFavorite tmp;

				tmp = prefs->favorites[
				    selection - 1];
				prefs->favorites[selection - 1] =
				    prefs->favorites[selection];
				prefs->favorites[selection] = tmp;
				selection--;
				prefs_save(prefs);

				fav_list_populate(prefs);
				fav_list_set_selection(selection);
			}
			break;
		case 9:  /* Move Down */
			selection =
			    fav_list_get_selection();
			if (selection >= 0 &&
			    selection <
			    prefs->favorite_count - 1) {
				GopherFavorite tmp;

				tmp = prefs->favorites[
				    selection + 1];
				prefs->favorites[selection + 1] =
				    prefs->favorites[selection];
				prefs->favorites[selection] = tmp;
				selection++;
				prefs_save(prefs);

				fav_list_populate(prefs);
				fav_list_set_selection(selection);
			}
			break;
		case 10:  /* Go To */
			selection =
			    fav_list_get_selection();
			if (selection >= 0 &&
			    selection <
			    prefs->favorite_count) {
				LDispose(g_fav_list);
				g_fav_list = 0L;
				DisposeDialog(dlg);
				favorites_rebuild_menu(prefs);
				do_navigate_url(
				    prefs->favorites[
				    selection].url);
				return;
			}
			break;
		}
	}

	LDispose(g_fav_list);
	g_fav_list = 0L;
	DisposeDialog(dlg);
	favorites_rebuild_menu(prefs);
}

void
favorites_menu_click(GeomysPrefs *prefs, short item)
{
	short fav_idx;

	fav_idx = item - FAV_FIRST_ENTRY;
	if (fav_idx >= 0 && fav_idx < prefs->favorite_count)
		do_navigate_url(prefs->favorites[fav_idx].url);
}
