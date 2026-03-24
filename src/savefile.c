/*
 * savefile.c - Save current page content to text file
 */

#ifdef GEOMYS_DOWNLOAD

#include <Quickdraw.h>
#include <Files.h>
#include <StandardFile.h>
#include <ToolUtils.h>
#include <Multiverse.h>
#include <Gestalt.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "gopher.h"
#include "gopher_types.h"
#include "macutil.h"
#include "sysutil.h"
#include "browser.h"
#include "content.h"
#include "savefile.h"
#include "imgparse.h"

/* g_gopher and g_app_state are macros in main.h
 * pointing to active_session fields */

/*
 * write_text_page - Write text page content to open file.
 * Each line terminated with CR (Mac line ending).
 * Returns noErr on success, or first file error encountered.
 */
static OSErr
write_text_page(short refNum, GopherState *gs)
{
	short i;
	long count;
	OSErr err;
	char cr = '\r';

	for (i = 0; i < gs->text_line_count; i++) {
		long start, end, len;
		const char *line;

		start = gs->text_lines[i];
		if (i + 1 < gs->text_line_count)
			end = gs->text_lines[i + 1];
		else
			end = gs->text_len;

		line = gs->text_buf + start;
		len = end - start;

		/* Strip trailing CR/LF */
		while (len > 0 &&
		    (line[len - 1] == '\r' || line[len - 1] == '\n'))
			len--;

		if (len > 0) {
			count = len;
			err = FSWrite(refNum, &count, line);
			if (err != noErr)
				return err;
		}

		/* Write CR */
		count = 1;
		err = FSWrite(refNum, &count, &cr);
		if (err != noErr)
			return err;
	}

	return noErr;
}

/*
 * write_directory_page - Write directory listing as readable text.
 * Info lines ('i' type): display text only.
 * Other types: "[LABEL]  Display Name" with gopher:// URI on next line.
 * Returns noErr on success, or first file error encountered.
 */
static OSErr
write_directory_page(short refNum, GopherState *gs)
{
	short i;
	long count;
	OSErr err;
	char line_buf[512];
	char uri[350];
	char cr = '\r';

	for (i = 0; i < gs->item_count; i++) {
		GopherItem *item = &gs->items[i];

		if (item->type == GOPHER_INFO) {
			/* Info line: just display text */
			short len = strlen(item->display);

			/* Trim trailing spaces */
			while (len > 0 &&
			    item->display[len - 1] == ' ')
				len--;

			if (len > 0) {
				count = len;
				err = FSWrite(refNum, &count,
				    item->display);
				if (err != noErr)
					return err;
			}
		} else {
			const char *label;
			short len;

			label = gopher_type_label(item->type);

			/* Format: [LABEL]  Display Name */
			len = snprintf(line_buf,
			    sizeof(line_buf), "[%s]  %s",
			    label, item->display);
			if (len >= (short)sizeof(line_buf))
				len = sizeof(line_buf) - 1;
			count = len;
			err = FSWrite(refNum, &count, line_buf);
			if (err != noErr)
				return err;

			/* Navigable items: append URI on next line */
			if (gopher_type_navigable(item->type)) {
				count = 1;
				err = FSWrite(refNum, &count, &cr);
				if (err != noErr)
					return err;

				gopher_build_uri(uri, sizeof(uri),
				    item->host, item->port,
				    item->type, item->selector);

				len = snprintf(line_buf,
				    sizeof(line_buf),
				    "    %s", uri);
				if (len >= (short)sizeof(line_buf))
					len = sizeof(line_buf) - 1;
				count = len;
				err = FSWrite(refNum, &count,
				    line_buf);
				if (err != noErr)
					return err;
			}
		}

		/* Write CR line ending */
		count = 1;
		err = FSWrite(refNum, &count, &cr);
		if (err != noErr)
			return err;
	}

	return noErr;
}

void
do_save_page(void)
{
	GopherState *gs = &g_gopher;
	Str255 default_name;
	short refNum;
	OSErr err;
	long sysver;
	Boolean use_std_file = false;

	/* Early return if nothing to save or still loading */
	if (gs->page_type == PAGE_NONE ||
	    g_app_state != APP_STATE_IDLE)
		return;

	/* Build default filename from cur_title (max 31 chars) */
	if (gs->cur_title[0]) {
		short len = strlen(gs->cur_title);
		short i;

		if (len > 31)
			len = 31;
		default_name[0] = len;
		memcpy(&default_name[1], gs->cur_title, len);

		/* Sanitize colons (Mac path separator) */
		for (i = 1; i <= len; i++) {
			if (default_name[i] == ':')
				default_name[i] = '-';
		}
	} else if (gs->cur_host[0]) {
		short len = strlen(gs->cur_host);

		if (len > 31)
			len = 31;
		default_name[0] = len;
		memcpy(&default_name[1], gs->cur_host, len);
	} else {
		default_name[0] = 11;
		memcpy(&default_name[1], "Gopher Page", 11);
	}

	/* Check for System 7+ StandardPutFile */
	if (TrapAvailable(_GestaltDispatch) &&
	    Gestalt(gestaltSystemVersion, &sysver) == noErr &&
	    sysver >= 0x0700)
		use_std_file = true;

	if (use_std_file) {
		/* System 7: StandardPutFile with FSSpec */
		StandardFileReply sf_reply;

		StandardPutFile("\pSave page as:",
		    default_name, &sf_reply);

		if (!sf_reply.sfGood)
			return;

		/* Delete existing file (ignore error) */
		FSpDelete(&sf_reply.sfFile);

		/* Create new file */
		err = FSpCreate(&sf_reply.sfFile, 'ttxt',
		    'TEXT', smSystemScript);
		if (err != noErr) {
			show_error_alert(
			    "Could not create file. "
			    "The disk may be full or "
			    "locked.");
			return;
		}

		/* Open for writing */
		err = FSpOpenDF(&sf_reply.sfFile, fsWrPerm,
		    &refNum);
		if (err != noErr) {
			show_error_alert(
			    "Could not open file for "
			    "writing. It may be in use "
			    "by another application.");
			return;
		}

		if (gs->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
		    || gs->page_type == PAGE_HTML
#endif
		    )
			err = write_text_page(refNum, gs);
		else
			err = write_directory_page(refNum, gs);

		FSClose(refNum);
		FlushVol(0L, sf_reply.sfFile.vRefNum);

		if (err != noErr)
			show_error_alert(
			    "Error writing file. "
			    "The disk may be full.");
	} else {
		/* System 6: SFPutFile with SFReply */
		SFReply reply;
		Point where;

		where.h = 80;
		where.v = 80;

		SFPutFile(where, "\pSave page as:",
		    default_name, 0L, &reply);

		if (!reply.good)
			return;

		/* Delete existing file (ignore error) */
		FSDelete(reply.fName, reply.vRefNum);

		/* Create new file */
		err = Create(reply.fName, reply.vRefNum,
		    'ttxt', 'TEXT');
		if (err != noErr) {
			show_error_alert(
			    "Could not create file. "
			    "The disk may be full or "
			    "locked.");
			return;
		}

		/* Open for writing */
		err = FSOpen(reply.fName, reply.vRefNum,
		    &refNum);
		if (err != noErr) {
			show_error_alert(
			    "Could not open file for "
			    "writing. It may be in use "
			    "by another application.");
			return;
		}

		if (gs->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
		    || gs->page_type == PAGE_HTML
#endif
		    )
			err = write_text_page(refNum, gs);
		else
			err = write_directory_page(refNum, gs);

		FSClose(refNum);
		FlushVol(0L, reply.vRefNum);

		if (err != noErr)
			show_error_alert(
			    "Error writing file. "
			    "The disk may be full.");
	}
}

/*
 * derive_download_name - Extract filename from selector path.
 * Uses last path component, sanitizes colons, max 31 chars.
 * Falls back to display name if selector has no useful filename.
 */
static void
derive_download_name(Str255 name, const GopherItem *item)
{
	const char *src;
	const char *slash;
	short len, i;

	/* Find last path component from selector */
	src = item->selector;
	slash = 0L;
	for (i = 0; src[i]; i++) {
		if (src[i] == '/')
			slash = &src[i];
	}
	if (slash && slash[1])
		src = slash + 1;
	else if (src[0] == '\0' || src[0] == '/')
		src = item->display;

	len = strlen(src);
	if (len > 31)
		len = 31;
	if (len == 0) {
		/* Last resort */
		name[0] = 8;
		memcpy(&name[1], "download", 8);
		return;
	}

	name[0] = len;
	memcpy(&name[1], src, len);

	/* Sanitize colons (Mac path separator) */
	for (i = 1; i <= len; i++) {
		if (name[i] == ':')
			name[i] = '-';
	}
}

/*
 * dl_type_codes - Map Gopher type to Mac file type/creator codes.
 */
static void
dl_type_codes(char gopher_type, OSType *ftype, OSType *fcreator)
{
	switch (gopher_type) {
	case GOPHER_BINHEX:
	case GOPHER_UUENCODE:
		*ftype = 'TEXT';
		*fcreator = 'ttxt';
		break;
	case GOPHER_SOUND:
		*ftype = 'sfil';
		*fcreator = 'SCPL';
		break;
	case GOPHER_RTF:
		*ftype = 'TEXT';
		*fcreator = 'MSWD';
		break;
	case GOPHER_GIF:
		*ftype = 'GIFf';
		*fcreator = '8BIM';
		break;
	case GOPHER_PNG:
		*ftype = 'PNGf';
		*fcreator = '8BIM';
		break;
	case GOPHER_IMAGE:
		*ftype = '????';
		*fcreator = '8BIM';
		break;
	default:
		*ftype = '????';
		*fcreator = '????';
		break;
	}
}

/*
 * start_save_to_disk - Common implementation for file/image downloads.
 * Shows SFPutFile dialog, creates file, redraws window behind dialog,
 * then starts the Gopher connection. On failure, cleans up the file.
 * prompt: Pascal string for SFPutFile (e.g. "\pSave file as:")
 * status_msg: C string for status bar during download
 */
static void
start_save_to_disk(const GopherItem *item,
    const unsigned char *prompt, const char *status_msg)
{
	GopherState *gs = &g_gopher;
	Str255 default_name;
	short refNum;
	OSErr err;
	long sysver;
	Boolean use_std_file = false;
	OSType ftype, fcreator;

	/* Don't start a download while already loading */
	if (g_app_state != APP_STATE_IDLE)
		return;

	derive_download_name(default_name, item);
	dl_type_codes(item->type, &ftype, &fcreator);

	/* Check for System 7+ StandardPutFile */
	if (TrapAvailable(_GestaltDispatch) &&
	    Gestalt(gestaltSystemVersion, &sysver) == noErr &&
	    sysver >= 0x0700)
		use_std_file = true;

	if (use_std_file) {
		/* System 7: StandardPutFile with FSSpec */
		StandardFileReply sf_reply;

		StandardPutFile(prompt, default_name,
		    &sf_reply);

		if (!sf_reply.sfGood)
			return;

		/* Delete existing file (ignore error) */
		FSpDelete(&sf_reply.sfFile);

		/* Create new file */
		err = FSpCreate(&sf_reply.sfFile, fcreator,
		    ftype, smSystemScript);
		if (err != noErr) {
			show_error_alert(
			    "Could not create file. "
			    "The disk may be full or "
			    "locked.");
			return;
		}

		/* Open for writing */
		err = FSpOpenDF(&sf_reply.sfFile, fsWrPerm,
		    &refNum);
		if (err != noErr) {
			show_error_alert(
			    "Could not open file for "
			    "writing. It may be in use "
			    "by another application.");
			return;
		}

		/* Store info for cleanup */
		gs->dl_refnum = refNum;
		gs->dl_written = 0;
		gs->dl_error = false;
		gs->dl_vrefnum = sf_reply.sfFile.vRefNum;
		memcpy(gs->dl_filename, sf_reply.sfFile.name,
		    sf_reply.sfFile.name[0] + 1);
	} else {
		/* System 6: SFPutFile with SFReply */
		SFReply reply;
		Point where;

		where.h = 80;
		where.v = 80;

		SFPutFile(where, prompt,
		    default_name, 0L, &reply);

		if (!reply.good)
			return;

		/* Delete existing file (ignore error) */
		FSDelete(reply.fName, reply.vRefNum);

		/* Create new file */
		err = Create(reply.fName, reply.vRefNum,
		    fcreator, ftype);
		if (err != noErr) {
			show_error_alert(
			    "Could not create file. "
			    "The disk may be full or "
			    "locked.");
			return;
		}

		/* Open for writing */
		err = FSOpen(reply.fName, reply.vRefNum,
		    &refNum);
		if (err != noErr) {
			show_error_alert(
			    "Could not open file for "
			    "writing. It may be in use "
			    "by another application.");
			return;
		}

		/* Store info for cleanup */
		gs->dl_refnum = refNum;
		gs->dl_written = 0;
		gs->dl_error = false;
		gs->dl_vrefnum = reply.vRefNum;
		memcpy(gs->dl_filename, reply.fName,
		    reply.fName[0] + 1);
	}

	/* Redraw window behind dismissed SFPutFile dialog */
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		InvalRect(&g_window->portRect);
		BeginUpdate(g_window);
		content_mark_all_dirty();
		browser_draw(g_window);
		content_draw(g_window);
		content_update_scroll(g_window);
		{
			Rect clip_r;
			RgnHandle sc = NewRgn();

			GetClip(sc);
			SetRect(&clip_r,
			    g_window->portRect.right - 15,
			    g_window->portRect.bottom - 15,
			    g_window->portRect.right + 1,
			    g_window->portRect.bottom + 1);
			ClipRect(&clip_r);
			EraseRect(&clip_r);
			DrawGrowIcon(g_window);
			SetClip(sc);
			DisposeRgn(sc);
		}
		DrawControls(g_window);
		EndUpdate(g_window);
		SetPort(save);
	}

	/* Start the download connection */
	g_app_state = APP_STATE_LOADING;
	SetCursor(*GetCursor(watchCursor));

	browser_set_status(status_msg);
	{
		GrafPtr save;

		GetPort(&save);
		SetPort(g_window);
		browser_draw_status(g_window);
		SetPort(save);
	}

	if (!gopher_navigate(gs, item->host, item->port,
	    item->type, item->selector)) {
		/* Connection failed — clean up file */
		FSClose(refNum);
		FlushVol(0L, gs->dl_vrefnum);
		FSDelete(gs->dl_filename, gs->dl_vrefnum);
		gs->dl_refnum = 0;
		gs->dl_written = 0;
		gs->dl_vrefnum = 0;
		gs->dl_filename[0] = 0;

		g_app_state = APP_STATE_IDLE;
		InitCursor();
		browser_set_status("Connection failed");
		{
			GrafPtr save;

			GetPort(&save);
			SetPort(g_window);
			browser_draw_status(g_window);
			SetPort(save);
		}
	}
}

/*
 * do_download_file - Show SFPutFile, then start streaming download to disk.
 */
void
do_download_file(const GopherItem *item)
{
	start_save_to_disk(item, "\pSave file as:",
	    "Downloading\311");
}

/*
 * do_image_save - Show SFPutFile for an image item, then start
 * PAGE_IMAGE download which sniffs header and streams to disk.
 */
void
do_image_save(const GopherItem *item)
{
	start_save_to_disk(item, "\pSave image as:",
	    "Downloading image\311");
}

#endif /* GEOMYS_DOWNLOAD */
