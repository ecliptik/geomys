/*
 * dialogs.c - Dialog management for Geomys
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <Menus.h>
#include <Dialogs.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "dialogs.h"
#include "macutil.h"
#include "sysutil.h"

/* ---- Status window UI ---- */

/* Status window dimensions (centered on 512x342 screen) */
#define STATUS_WIN_W   320
#define STATUS_WIN_H    40

WindowPtr
conn_status_show(const char *msg)
{
	WindowPtr w;
	Rect r;
	Str255 title;
	GrafPtr save;
	Str255 ps;
	short len;

	SetRect(&r,
	    (512 - STATUS_WIN_W) / 2,
	    (342 - STATUS_WIN_H) / 2 + 20,
	    (512 + STATUS_WIN_W) / 2,
	    (342 + STATUS_WIN_H) / 2 + 20);
	title[0] = 0;
	w = NewWindow(0L, &r, title, true, dBoxProc,
	    (WindowPtr)-1L, false, 0L);
	if (w) {
		Rect clip_r;

		GetPort(&save);
		SetPort(w);
		TextFont(0);   /* Chicago */
		TextSize(12);
		SetRect(&clip_r, 0, 0, STATUS_WIN_W, STATUS_WIN_H);
		ClipRect(&clip_r);
		len = strlen(msg);
		if (len > 255) len = 255;
		ps[0] = len;
		memcpy(ps + 1, msg, len);
		MoveTo(10, 26);
		DrawString(ps);
		SetPort(save);
	}
	return w;
}

void
conn_status_update(WindowPtr w, const char *msg)
{
	GrafPtr save;
	Rect r;
	Str255 ps;
	short len;

	if (!w) return;
	GetPort(&save);
	SetPort(w);
	SetRect(&r, 0, 0, STATUS_WIN_W, STATUS_WIN_H);
	ClipRect(&r);
	EraseRect(&r);
	len = strlen(msg);
	if (len > 255) len = 255;
	ps[0] = len;
	memcpy(ps + 1, msg, len);
	MoveTo(10, 26);
	DrawString(ps);
	SetPort(save);
}

void
conn_status_close(WindowPtr w)
{
	if (w)
		DisposeWindow(w);
}

/* ---- Default button outline ---- */

/* Draw a 3-pixel rounded rect outline around the default button (item 1) */
pascal void
draw_default_button(WindowPtr dlg, short item)
{
	short item_type;
	Handle item_h;
	Rect item_rect, outline_r;

	(void)item;  /* unused - always outline item 1 */
	GetDialogItem((DialogPtr)dlg, 1, &item_type, &item_h, &item_rect);
	outline_r = item_rect;
	InsetRect(&outline_r, -4, -4);
	PenSize(3, 3);
	FrameRoundRect(&outline_r, 16, 16);
	PenNormal();
}

/* Register the default button outline UserItem in a dialog */
void
setup_default_button_outline(DialogPtr dlg, short outline_item)
{
	short item_type;
	Handle item_h;
	Rect item_rect;

	GetDialogItem(dlg, outline_item, &item_type, &item_h, &item_rect);
	SetDialogItem(dlg, outline_item, userItem,
	    (Handle)draw_default_button, &item_rect);
}

/* ---- Standard dialog filter ---- */

/* Simple dialog filter for Return=OK, Cmd+.=Cancel */
pascal Boolean
std_dlg_filter(DialogPtr dlg, EventRecord *evt, short *item)
{
	(void)dlg;
	if (evt->what == keyDown) {
		char key = evt->message & charCodeMask;
		if (key == '\r' || key == '\n' || key == 0x03) {
			*item = 1;  /* OK button */
			return true;
		}
		if ((evt->modifiers & cmdKey) && key == '.') {
			*item = 2;  /* Cancel button */
			return true;
		}
		if (key == 0x1B) {  /* Escape */
			*item = 2;
			return true;
		}
	}
	return false;
}

/* ---- About dialog ---- */

void
do_about(void)
{
	DialogPtr dlg;
	short item;
	char machine[32];
	char running_on[64];
	short item_type;
	Handle item_h;
	Rect item_rect;
	Str255 pstr;

	dlg = GetNewDialog(DLOG_ABOUT_ID, 0L, (WindowPtr)-1L);
	if (!dlg)
		return;

	/* Set machine type in item 4 */
	get_machine_name(machine, sizeof(machine));
	snprintf(running_on, sizeof(running_on), "Running on %s",
	    machine);
	c2pstr(pstr, running_on);
	GetDialogItem(dlg, 4, &item_type, &item_h, &item_rect);
	SetDialogItemText(item_h, pstr);

	/* Register default button outline */
	setup_default_button_outline(dlg, 8);

	ModalDialog((ModalFilterUPP)std_dlg_filter, &item);
	DisposeDialog(dlg);
}
