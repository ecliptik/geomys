/*
 * settings.c - Preferences persistence for Geomys
 *
 * System 7+: stored in Preferences folder.
 * System 6: stored at root of default volume.
 */

#include <Files.h>
#include <Memory.h>
#include <string.h>
#include "settings.h"
#include "sysutil.h"
#include "tcp.h"
#include "main.h"

#define PREFS_FILENAME	"\pGeomys Preferences"
#define PREFS_TEMPNAME	"\pGeomys Prefs (tmp)"

static OSErr
prefs_get_location(short *vRefNum, long *dirID)
{
	long response;

	if (Gestalt('fold', &response) == noErr) {
		OSErr err;
		err = FindFolder(kOnSystemDisk, kPreferencesFolderType,
		    true, vRefNum, dirID);
		if (err == noErr)
			return noErr;
	}

	/* System 6 fallback: default volume root */
	*dirID = 0;
	return GetVol(0L, vRefNum);
}

void
prefs_defaults(GeomysPrefs *prefs)
{
	memset(prefs, 0, sizeof(GeomysPrefs));
	prefs->version = PREFS_VERSION;
	strncpy(prefs->home_url, DEFAULT_HOME_URL,
	    sizeof(prefs->home_url) - 1);
	prefs->home_url[sizeof(prefs->home_url) - 1] = '\0';
	strncpy(prefs->dns_server, "1.1.1.1",
	    sizeof(prefs->dns_server) - 1);
	prefs->dns_server[sizeof(prefs->dns_server) - 1] = '\0';
	prefs->font_id = 4;    /* Monaco */
	prefs->font_size = 9;
	prefs->favorite_count = 0;
	prefs->page_style = STYLE_TURBOGOPHER;
	prefs->show_details = 1;
	prefs->theme_id = 0;  /* THEME_LIGHT */
	prefs->show_status_bar = 1;  /* visible by default */
	prefs->gopher_plus = 0;      /* off by default */
}

void
prefs_load(GeomysPrefs *prefs)
{
	HParamBlockRec pb;
	long count;
	short vRefNum;
	long dirID;
	OSErr err;

	prefs_defaults(prefs);

	err = prefs_get_location(&vRefNum, &dirID);
	if (err != noErr)
		return;

	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_FILENAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioPermssn = fsRdPerm;
	pb.fileParam.ioDirID = dirID;
	err = PBHOpenSync(&pb);
	if (err != noErr)
		return;

	count = sizeof(GeomysPrefs);
	err = FSRead(pb.ioParam.ioRefNum, &count, (Ptr)prefs);
	FSClose(pb.ioParam.ioRefNum);

	if (err != noErr && err != eofErr) {
		prefs_defaults(prefs);
		return;
	}

	/* Force null termination on all string fields */
	prefs->home_url[sizeof(prefs->home_url) - 1] = '\0';
	prefs->dns_server[sizeof(prefs->dns_server) - 1] = '\0';
	{
		short i;
		for (i = 0; i < MAX_FAVORITES; i++) {
			prefs->favorites[i].name[
			    sizeof(prefs->favorites[i].name) - 1] = '\0';
			prefs->favorites[i].url[
			    sizeof(prefs->favorites[i].url) - 1] = '\0';
		}
	}

	/* Validate DNS server IP */
	if (prefs->dns_server[0] == '\0' ||
	    prefs->dns_server[0] == '.' ||
	    ip2long(prefs->dns_server) == 0) {
		strncpy(prefs->dns_server, "1.1.1.1",
		    sizeof(prefs->dns_server) - 1);
		prefs->dns_server[sizeof(prefs->dns_server) - 1] = '\0';
	}

	if (prefs->version != PREFS_VERSION) {
		/* v5 -> v6: page styles changed from 3 to 2 */
		if (prefs->version == 5) {
			if (prefs->page_style > 1)
				prefs->page_style = 0;
			prefs->version = 6;
		}
		/* v6 -> v7: added gopher_plus toggle */
		if (prefs->version == 6) {
			prefs->gopher_plus = 0;
			prefs->version = 7;
		}
		/* v7 -> v8: page_style renumbered for 5 historical-client
		 * styles. Old 0 (text labels) maps to PC Gopher II (closest
		 * label-style peer); old 1 (icons) maps to TurboGopher. */
		if (prefs->version == 7) {
			if (prefs->page_style == 0)
				prefs->page_style = STYLE_PCGOPHER;
			else if (prefs->page_style == 1)
				prefs->page_style = STYLE_TURBOGOPHER;
			else
				prefs->page_style = STYLE_TURBOGOPHER;
			prefs->version = PREFS_VERSION;
		}
		if (prefs->version != PREFS_VERSION)
			prefs_defaults(prefs);
	}

	/* Defensive clamp on anything out of range after migrations. */
	if (prefs->page_style < STYLE_TURBOGOPHER ||
	    prefs->page_style > STYLE_RFC1436)
		prefs->page_style = STYLE_TURBOGOPHER;
}

/* Delete the scratch temp file; ignore errors (best-effort cleanup). */
static void
prefs_delete_temp(short vRefNum, long dirID)
{
	HParamBlockRec pb;

	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_TEMPNAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.fileParam.ioDirID = dirID;
	PBHDeleteSync(&pb);
}

/*
 * prefs_save - persist preferences atomically.
 *
 * Writes to a scratch temp file and verifies every step; only once the temp
 * holds a complete, flushed copy is the old "Geomys Preferences" replaced
 * (delete + rename).  Any failure leaves the existing prefs untouched, so a
 * disk-full / locked-volume / interrupted write can no longer wipe the home
 * URL, DNS server, favorites, theme and fonts.  Returns no error (the
 * prototype is void and all callers ignore it), but never destroys good data.
 */
void
prefs_save(GeomysPrefs *prefs)
{
	HParamBlockRec pb;
	long count;
	short vRefNum;
	long dirID;
	short refNum;
	OSErr err;

	err = prefs_get_location(&vRefNum, &dirID);
	if (err != noErr)
		return;

	prefs->version = PREFS_VERSION;

	/* Clear any stale temp file left by a previous interrupted save. */
	prefs_delete_temp(vRefNum, dirID);

	/* Create the temp file. */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_TEMPNAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.fileParam.ioDirID = dirID;
	err = PBHCreateSync(&pb);
	if (err != noErr)
		return;

	/* Set type and creator so the final file is tagged correctly. */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_TEMPNAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.fileParam.ioDirID = dirID;
	if (PBHGetFInfoSync(&pb) == noErr) {
		pb.fileParam.ioDirID = dirID;
		pb.fileParam.ioFlFndrInfo.fdType = 'pref';
		pb.fileParam.ioFlFndrInfo.fdCreator = 'GEOM';
		PBHSetFInfoSync(&pb);
	}

	/* Open the temp file for writing. */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_TEMPNAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioPermssn = fsWrPerm;
	pb.fileParam.ioDirID = dirID;
	err = PBHOpenSync(&pb);
	if (err != noErr) {
		prefs_delete_temp(vRefNum, dirID);
		return;
	}
	refNum = pb.ioParam.ioRefNum;

	/* Write the full record; verify both the error and the byte count. */
	count = sizeof(GeomysPrefs);
	err = FSWrite(refNum, &count, (Ptr)prefs);
	if (err == noErr && count != (long)sizeof(GeomysPrefs))
		err = ioErr;

	/* Close always; a close error can still signal a failed flush. */
	{
		OSErr close_err = FSClose(refNum);
		if (err == noErr)
			err = close_err;
	}

	if (err != noErr) {
		/* Write/flush failed — discard temp, keep existing prefs. */
		prefs_delete_temp(vRefNum, dirID);
		return;
	}

	/* Temp now holds a complete, verified copy.  Remove the old file. */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_FILENAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.fileParam.ioDirID = dirID;
	err = PBHDeleteSync(&pb);
	if (err != noErr && err != fnfErr) {
		/* Old file is locked/busy — leave it intact, drop the temp. */
		prefs_delete_temp(vRefNum, dirID);
		return;
	}

	/* Rename the temp over the real prefs name. */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_TEMPNAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioMisc = (Ptr)PREFS_FILENAME;
	pb.fileParam.ioDirID = dirID;
	PBHRenameSync(&pb);
}
