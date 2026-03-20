/*
 * savefile.c - Save current page content to text file
 */

#ifdef GEOMYS_DOWNLOAD

#include <Quickdraw.h>
#include <Files.h>
#include <StandardFile.h>
#include <Multiverse.h>
#include <Gestalt.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "gopher.h"
#include "gopher_types.h"
#include "macutil.h"
#include "sysutil.h"
#include "savefile.h"

/* External references to main.c globals */
extern GopherState g_gopher;
extern short g_app_state;

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
			len = sprintf(line_buf, "[%s]  %s",
			    label, item->display);
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

				len = sprintf(line_buf, "    %s",
				    uri);
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
			    "Could not create file.");
			return;
		}

		/* Open for writing */
		err = FSpOpenDF(&sf_reply.sfFile, fsWrPerm,
		    &refNum);
		if (err != noErr) {
			show_error_alert(
			    "Could not open file for writing.");
			return;
		}

		if (gs->page_type == PAGE_TEXT)
			err = write_text_page(refNum, gs);
		else
			err = write_directory_page(refNum, gs);

		FSClose(refNum);
		FlushVol(0L, sf_reply.sfFile.vRefNum);

		if (err != noErr)
			show_error_alert("Error writing file.");
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
			    "Could not create file.");
			return;
		}

		/* Open for writing */
		err = FSOpen(reply.fName, reply.vRefNum,
		    &refNum);
		if (err != noErr) {
			show_error_alert(
			    "Could not open file for writing.");
			return;
		}

		if (gs->page_type == PAGE_TEXT)
			err = write_text_page(refNum, gs);
		else
			err = write_directory_page(refNum, gs);

		FSClose(refNum);
		FlushVol(0L, reply.vRefNum);

		if (err != noErr)
			show_error_alert("Error writing file.");
	}
}

#endif /* GEOMYS_DOWNLOAD */
