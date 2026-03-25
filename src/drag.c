/*
 * drag.c - Drag Manager support for URL drag-and-drop (System 7.5+)
 *
 * Drag OUT: user drags a link from content area to Desktop or
 *           another app, creating a text clipping with the URL.
 * Drag IN:  user drops a text clipping or URL into the address
 *           bar, navigating to the dropped URL.
 *
 * All functions are Gestalt-gated — no-ops on System 6 / 7.0-7.1.
 */

#ifdef GEOMYS_DRAG

#include <Quickdraw.h>
#include <Events.h>
#include <Windows.h>
#include <Gestalt.h>
#include <Memory.h>
#include <Multiverse.h>
#include <string.h>

#include "drag.h"
#include "main.h"
#include "browser.h"

/* Drag Manager availability flag */
static Boolean g_drag_available = false;

/* Installed handler UPPs (needed for removal) */
static DragTrackingHandlerUPP g_tracking_upp = 0L;
static DragReceiveHandlerUPP  g_receive_upp = 0L;

/* Track whether address bar is currently drag-highlighted */
static Boolean g_drag_hilited = false;

/*
 * drag_tracking_handler - Highlight address bar when drag enters.
 * Called by Drag Manager during drag tracking over our window.
 */
static pascal OSErr
drag_tracking_handler(DragTrackingMessage message,
    WindowPtr theWindow, void *handlerRefCon,
    DragReference theDrag)
{
	(void)handlerRefCon;

	switch (message) {
	case kDragTrackingEnterWindow:
		g_drag_hilited = false;
		break;
	case kDragTrackingInWindow:
		{
			Rect addr_r;
			Point mouse, pinned;

			browser_get_addr_rect(&addr_r);
			GetDragMouse(theDrag, &mouse, &pinned);
			SetPort(theWindow);
			GlobalToLocal(&mouse);

			if (PtInRect(mouse, &addr_r)) {
				if (!g_drag_hilited) {
					RgnHandle rgn = NewRgn();
					RectRgn(rgn, &addr_r);
					ShowDragHilite(theDrag,
					    rgn, true);
					DisposeRgn(rgn);
					g_drag_hilited = true;
				}
			} else {
				if (g_drag_hilited) {
					HideDragHilite(theDrag);
					g_drag_hilited = false;
				}
			}
		}
		break;
	case kDragTrackingLeaveWindow:
		if (g_drag_hilited) {
			HideDragHilite(theDrag);
			g_drag_hilited = false;
		}
		break;
	}

	return noErr;
}

/*
 * drag_receive_handler - Accept dropped text as URL.
 * Extracts 'TEXT' flavor data, checks for gopher:// prefix,
 * and navigates if valid.
 */
static pascal OSErr
drag_receive_handler(WindowPtr theWindow, void *handlerRefCon,
    DragReference theDrag)
{
	unsigned short count;
	unsigned short i;
	ItemReference item_ref;
	Size data_size;
	char url[300];
	OSErr err;

	(void)theWindow;
	(void)handlerRefCon;

	err = CountDragItems(theDrag, &count);
	if (err != noErr)
		return err;

	/* Process first item only */
	for (i = 1; i <= count && i <= 1; i++) {
		err = GetDragItemReferenceNumber(theDrag, i,
		    &item_ref);
		if (err != noErr)
			continue;

		/* Check for TEXT flavor */
		data_size = sizeof(url) - 1;
		err = GetFlavorData(theDrag, item_ref, 'TEXT',
		    url, &data_size, 0);
		if (err != noErr)
			continue;

		url[data_size] = '\0';

		/* Strip trailing whitespace */
		while (data_size > 0 &&
		    (url[data_size - 1] == '\r' ||
		     url[data_size - 1] == '\n' ||
		     url[data_size - 1] == ' '))
			url[--data_size] = '\0';

		/* Navigate if it's a gopher URL */
		if (data_size > 9 &&
		    strncmp(url, "gopher://", 9) == 0) {
			do_navigate_url(url);
			return noErr;
		}
	}

	return dragNotAcceptedErr;
}

void
drag_init(void)
{
	long resp;

	if (Gestalt(gestaltDragMgrAttr, &resp) == noErr &&
	    (resp & (1L << gestaltDragMgrPresent)))
		g_drag_available = true;

	if (g_drag_available) {
		g_tracking_upp =
		    (DragTrackingHandlerUPP)drag_tracking_handler;
		g_receive_upp =
		    (DragReceiveHandlerUPP)drag_receive_handler;
	}
}

void
drag_install_handlers(WindowPtr win)
{
	if (!g_drag_available || !win)
		return;

	InstallTrackingHandler(g_tracking_upp, win, 0L);
	InstallReceiveHandler(g_receive_upp, win, 0L);
}

void
drag_remove_handlers(WindowPtr win)
{
	if (!g_drag_available || !win)
		return;

	RemoveTrackingHandler(g_tracking_upp, win);
	RemoveReceiveHandler(g_receive_upp, win);
}

Boolean
drag_start_url(WindowPtr win, EventRecord *event,
    const char *url, Rect *item_rect)
{
	DragReference drag_ref;
	RgnHandle drag_rgn;
	OSErr err;

	if (!g_drag_available || !url || !url[0])
		return false;

	err = NewDrag(&drag_ref);
	if (err != noErr)
		return false;

	/* Add TEXT flavor with the gopher:// URL */
	err = AddDragItemFlavor(drag_ref, 1, 'TEXT',
	    url, strlen(url), 0);
	if (err != noErr) {
		DisposeDrag(drag_ref);
		return false;
	}

	/* Set item bounds */
	SetDragItemBounds(drag_ref, 1, item_rect);

	/* Build outline region: border of item rect */
	drag_rgn = NewRgn();
	{
		RgnHandle inner = NewRgn();
		RgnHandle outer = NewRgn();

		RectRgn(outer, item_rect);
		*inner = *outer;
		InsetRgn(inner, 1, 1);
		DiffRgn(outer, inner, drag_rgn);
		DisposeRgn(inner);
		DisposeRgn(outer);
	}

	/* Track the drag — this blocks until drop or cancel */
	err = TrackDrag(drag_ref, event, drag_rgn);

	DisposeRgn(drag_rgn);
	DisposeDrag(drag_ref);

	(void)win;

	return (err == noErr);
}

Boolean
drag_available(void)
{
	return g_drag_available;
}

#endif /* GEOMYS_DRAG */
