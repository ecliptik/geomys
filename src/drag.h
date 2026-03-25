/*
 * drag.h - Drag Manager support for URL drag-and-drop (System 7.5+)
 *
 * Provides drag-out (links to Desktop/other apps) and drag-in
 * (text clippings/URLs into address bar). Gestalt-gated at runtime.
 */

#ifndef DRAG_H
#define DRAG_H

#ifdef GEOMYS_DRAG

#include <Quickdraw.h>
#include <Events.h>
#include <Windows.h>
#include <Multiverse.h>

/* ===== Drag Manager types (not in Retro68 headers) ===== */

typedef struct OpaqueDragRef *DragReference;
typedef unsigned long ItemReference;
typedef OSType FlavorType;
typedef unsigned short FlavorFlags;

typedef unsigned short DragTrackingMessage;

enum {
	kDragTrackingEnterHandler = 1,
	kDragTrackingEnterWindow  = 2,
	kDragTrackingInWindow     = 3,
	kDragTrackingLeaveWindow  = 4,
	kDragTrackingLeaveHandler = 5
};

/* Gestalt selector for Drag Manager */
#ifndef gestaltDragMgrAttr
#define gestaltDragMgrAttr   'drag'
#define gestaltDragMgrPresent 0
#endif

/* Error codes */
#ifndef dragNotAcceptedErr
#define dragNotAcceptedErr   (-1850)
#endif

/* Callback UPPs — on 68K, plain function pointers */
typedef pascal OSErr (*DragTrackingHandlerUPP)(
    DragTrackingMessage message, WindowPtr theWindow,
    void *handlerRefCon, DragReference theDrag);
typedef pascal OSErr (*DragReceiveHandlerUPP)(
    WindowPtr theWindow, void *handlerRefCon,
    DragReference theDrag);

/* ===== Drag Manager trap functions (0xABED dispatch) ===== */

pascal OSErr NewDrag(DragReference *theDragRef)
    M68K_INLINE(0x7001, 0xABED);
pascal OSErr DisposeDrag(DragReference theDragRef)
    M68K_INLINE(0x7002, 0xABED);
pascal OSErr AddDragItemFlavor(DragReference theDrag,
    ItemReference theItemRef, FlavorType theType,
    const void *dataPtr, Size dataSize, FlavorFlags theFlags)
    M68K_INLINE(0x7003, 0xABED);
pascal OSErr SetDragItemBounds(DragReference theDrag,
    ItemReference theItemRef, const Rect *itemBounds)
    M68K_INLINE(0x7004, 0xABED);
pascal OSErr TrackDrag(DragReference theDrag,
    const EventRecord *theEvent, RgnHandle theRegion)
    M68K_INLINE(0x7005, 0xABED);
pascal OSErr CountDragItems(DragReference theDrag,
    unsigned short *numItems)
    M68K_INLINE(0x7006, 0xABED);
pascal OSErr GetDragItemReferenceNumber(DragReference theDrag,
    unsigned short index, ItemReference *theItemRef)
    M68K_INLINE(0x7007, 0xABED);
pascal OSErr GetFlavorDataSize(DragReference theDrag,
    ItemReference theItemRef, FlavorType theType,
    Size *dataSize)
    M68K_INLINE(0x700B, 0xABED);
pascal OSErr GetFlavorData(DragReference theDrag,
    ItemReference theItemRef, FlavorType theType,
    void *dataPtr, Size *dataSize, unsigned long dataOffset)
    M68K_INLINE(0x700C, 0xABED);
pascal OSErr ShowDragHilite(DragReference theDrag,
    RgnHandle hiliteFrame, Boolean inside)
    M68K_INLINE(0x7009, 0xABED);
pascal OSErr HideDragHilite(DragReference theDrag)
    M68K_INLINE(0x700A, 0xABED);
pascal OSErr GetDragMouse(DragReference theDrag,
    Point *mouse, Point *pinnedMouse)
    M68K_INLINE(0x700D, 0xABED);
pascal OSErr InstallTrackingHandler(
    DragTrackingHandlerUPP trackingHandler,
    WindowPtr theWindow, void *handlerRefCon)
    M68K_INLINE(0x7013, 0xABED);
pascal OSErr InstallReceiveHandler(
    DragReceiveHandlerUPP receiveHandler,
    WindowPtr theWindow, void *handlerRefCon)
    M68K_INLINE(0x7014, 0xABED);
pascal OSErr RemoveTrackingHandler(
    DragTrackingHandlerUPP trackingHandler,
    WindowPtr theWindow)
    M68K_INLINE(0x7015, 0xABED);
pascal OSErr RemoveReceiveHandler(
    DragReceiveHandlerUPP receiveHandler,
    WindowPtr theWindow)
    M68K_INLINE(0x7016, 0xABED);

/* ===== Geomys drag API ===== */

/* Initialize drag support — call once at startup.
 * Checks Gestalt for Drag Manager availability. */
void drag_init(void);

/* Install drag tracking/receive handlers on a window.
 * Call after window creation. No-op if Drag Manager absent. */
void drag_install_handlers(WindowPtr win);

/* Remove drag handlers from a window.
 * Call before window disposal. */
void drag_remove_handlers(WindowPtr win);

/* Start a drag-out operation with a URL.
 * Called from content click handler when user drags a link.
 * Returns true if drag was initiated. */
Boolean drag_start_url(WindowPtr win, EventRecord *event,
    const char *url, Rect *item_rect);

/* Returns true if Drag Manager is available */
Boolean drag_available(void);

#else /* !GEOMYS_DRAG */

#define drag_init()                  ((void)0)
#define drag_install_handlers(w)     ((void)0)
#define drag_remove_handlers(w)      ((void)0)
#define drag_available()             (false)

#endif /* GEOMYS_DRAG */

#endif /* DRAG_H */
