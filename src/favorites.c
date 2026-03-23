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

	dlg = GetNewDialog(DLOG_EDIT_FAV_ID, 0L, 0L);
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

/*
 * Draw the favorites list in the dialog.
 * Highlights the selected item if selection >= 0.
 */
/*
 * Draw the favorites list — matches Flynn's bookmark list style.
 * Uses dialog default font (Chicago 12), 16px row height.
 */
static void
draw_fav_list(DialogPtr dlg, GeomysPrefs *prefs,
    Rect *list_rect, short selection)
{
	short i, y;
	short len;
	Rect inner;
	RgnHandle save_clip;

	(void)dlg;

	EraseRect(list_rect);
	FrameRect(list_rect);

	inner = *list_rect;
	InsetRect(&inner, 1, 1);

	/* Clip to list interior */
	save_clip = NewRgn();
	GetClip(save_clip);
	ClipRect(&inner);

	for (i = 0; i < prefs->favorite_count; i++) {
		y = inner.top + 2 + i * 16;
		if (y + 14 > inner.bottom)
			break;

		MoveTo(inner.left + 4, y + 12);
		len = strlen(prefs->favorites[i].name);
		DrawText(prefs->favorites[i].name, 0, len);

		if (i == selection) {
			Rect sel_r;

			SetRect(&sel_r, inner.left, y - 1,
			    inner.right, y + 15);
			InvertRect(&sel_r);
		}
	}

	SetClip(save_clip);
	DisposeRgn(save_clip);
}

void
favorites_manage(GeomysPrefs *prefs)
{
	DialogPtr dlg;
	short item;
	short selection = -1;
	short item_type;
	Handle item_h;
	Rect list_rect;
	GrafPtr save;
	Boolean done = false;

	dlg = GetNewDialog(DLOG_FAVORITES_ID, 0L, 0L);
	if (!dlg)
		return;
	center_dialog_on_screen(dlg);
	SelectWindow((WindowPtr)dlg);

	setup_default_button_outline(dlg, 6);

	/* Get list area rect */
	GetDialogItem(dlg, 5, &item_type, &item_h, &list_rect);

	/* Draw initial list */
	GetPort(&save);
	SetPort((WindowPtr)dlg);
	draw_fav_list(dlg, prefs, &list_rect, selection);
	SetPort(save);

	while (!done) {
		ModalDialog((ModalFilterUPP)std_dlg_filter, &item);

		switch (item) {
		case 1:  /* Done */
			done = true;
			break;
		case 2:  /* Add */
			if (prefs->favorite_count < MAX_FAVORITES) {
				DialogPtr edlg;
				short eitem;
				short eit;
				Handle eih;
				Rect eir;
				Str255 eps;

				edlg = GetNewDialog(DLOG_EDIT_FAV_ID,
				    0L, (WindowPtr)-1L);
				if (edlg) {
					center_dialog_on_screen(edlg);
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
						if (len >= 63) len = 63;
						memcpy(fav->name,
						    eps + 1, len);
						fav->name[len] = '\0';

						GetDialogItem(edlg, 6,
						    &eit, &eih, &eir);
						GetDialogItemText(
						    eih, eps);
						len = eps[0];
						if (len >= 255) len = 255;
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

					GetPort(&save);
					SetPort((WindowPtr)dlg);
					draw_fav_list(dlg, prefs,
					    &list_rect, selection);
					SetPort(save);
				}
			}
			break;
		case 3:  /* Edit */
			if (selection >= 0 &&
			    selection < prefs->favorite_count) {
				DialogPtr edlg;
				short eitem;
				short eit;
				Handle eih;
				Rect eir;
				Str255 eps;

				edlg = GetNewDialog(DLOG_EDIT_FAV_ID,
				    0L, (WindowPtr)-1L);
				if (edlg) {
					center_dialog_on_screen(edlg);
					/* Pre-fill fields */
					c2pstr(eps,
					    prefs->favorites[
					    selection].name);
					GetDialogItem(edlg, 4,
					    &eit, &eih, &eir);
					SetDialogItemText(eih, eps);

					c2pstr(eps,
					    prefs->favorites[
					    selection].url);
					GetDialogItem(edlg, 6,
					    &eit, &eih, &eir);
					SetDialogItemText(eih, eps);

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

						fav = &prefs->favorites[
						    selection];

						GetDialogItem(edlg, 4,
						    &eit, &eih, &eir);
						GetDialogItemText(
						    eih, eps);
						len = eps[0];
						if (len >= 63) len = 63;
						memcpy(fav->name,
						    eps + 1, len);
						fav->name[len] = '\0';

						GetDialogItem(edlg, 6,
						    &eit, &eih, &eir);
						GetDialogItemText(
						    eih, eps);
						len = eps[0];
						if (len >= 255) len = 255;
						memcpy(fav->url,
						    eps + 1, len);
						fav->url[len] = '\0';

						prefs_save(prefs);
					}
					DisposeDialog(edlg);

					GetPort(&save);
					SetPort((WindowPtr)dlg);
					draw_fav_list(dlg, prefs,
					    &list_rect, selection);
					SetPort(save);
				}
			}
			break;
		case 5:  /* List click */
		{
			Point mouse;
			short clicked;

			GetPort(&save);
			SetPort((WindowPtr)dlg);
			GetMouse(&mouse);

			clicked = (mouse.v - list_rect.top - 3)
			    / 16;
			if (clicked >= 0 &&
			    clicked < prefs->favorite_count) {
				selection = clicked;
				draw_fav_list(dlg, prefs,
				    &list_rect, selection);
			}

			SetPort(save);
			break;
		}
		case 7:  /* Delete */
			if (selection >= 0 &&
			    selection < prefs->favorite_count) {
				short i;

				ParamText(
				    "\pDelete this favorite?",
				    "\p", "\p", "\p");
				if (CautionAlert(129, 0L) == 2) {
					for (i = selection;
					    i < prefs->favorite_count
					    - 1; i++)
						prefs->favorites[i] =
						    prefs->favorites[
						    i + 1];
					prefs->favorite_count--;
					prefs_save(prefs);
					if (selection >=
					    prefs->favorite_count)
						selection = -1;

					GetPort(&save);
					SetPort((WindowPtr)dlg);
					draw_fav_list(dlg, prefs,
					    &list_rect, selection);
					SetPort(save);
				}
			}
			break;
		case 8:  /* Move Up */
			if (selection > 0 &&
			    selection < prefs->favorite_count) {
				GopherFavorite tmp;

				tmp = prefs->favorites[selection - 1];
				prefs->favorites[selection - 1] =
				    prefs->favorites[selection];
				prefs->favorites[selection] = tmp;
				selection--;
				prefs_save(prefs);

				GetPort(&save);
				SetPort((WindowPtr)dlg);
				draw_fav_list(dlg, prefs,
				    &list_rect, selection);
				SetPort(save);
			}
			break;
		case 9:  /* Move Down */
			if (selection >= 0 &&
			    selection < prefs->favorite_count - 1) {
				GopherFavorite tmp;

				tmp = prefs->favorites[selection + 1];
				prefs->favorites[selection + 1] =
				    prefs->favorites[selection];
				prefs->favorites[selection] = tmp;
				selection++;
				prefs_save(prefs);

				GetPort(&save);
				SetPort((WindowPtr)dlg);
				draw_fav_list(dlg, prefs,
				    &list_rect, selection);
				SetPort(save);
			}
			break;
		case 10:  /* Go To */
			if (selection >= 0 &&
			    selection < prefs->favorite_count) {
				DisposeDialog(dlg);
				favorites_rebuild_menu(prefs);
				do_navigate_url(
				    prefs->favorites[selection].url);
				return;
			}
			break;
		}
	}

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
