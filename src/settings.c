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
	prefs->page_style = STYLE_TEXT;
	prefs->show_details = 1;
	prefs->theme_id = 0;  /* THEME_LIGHT */
	prefs->show_status_bar = 1;  /* visible by default */
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
			if (prefs->page_style > STYLE_ICONS)
				prefs->page_style = STYLE_TEXT;
			prefs->version = PREFS_VERSION;
		} else {
			prefs_defaults(prefs);
		}
	}
}

void
prefs_save(GeomysPrefs *prefs)
{
	HParamBlockRec pb;
	long count;
	short vRefNum;
	long dirID;
	OSErr err;

	err = prefs_get_location(&vRefNum, &dirID);
	if (err != noErr)
		return;

	/* Delete existing file */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_FILENAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.fileParam.ioDirID = dirID;
	PBHDeleteSync(&pb);

	/* Create new file */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_FILENAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.fileParam.ioDirID = dirID;
	err = PBHCreateSync(&pb);
	if (err != noErr)
		return;

	/* Set type and creator */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_FILENAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.fileParam.ioDirID = dirID;
	err = PBHGetFInfoSync(&pb);
	if (err != noErr)
		return;
	pb.fileParam.ioDirID = dirID;
	pb.fileParam.ioFlFndrInfo.fdType = 'pref';
	pb.fileParam.ioFlFndrInfo.fdCreator = 'GEOM';
	PBHSetFInfoSync(&pb);

	/* Open and write */
	memset(&pb, 0, sizeof(pb));
	pb.ioParam.ioNamePtr = (StringPtr)PREFS_FILENAME;
	pb.ioParam.ioVRefNum = vRefNum;
	pb.ioParam.ioPermssn = fsWrPerm;
	pb.fileParam.ioDirID = dirID;
	err = PBHOpenSync(&pb);
	if (err != noErr)
		return;

	prefs->version = PREFS_VERSION;
	count = sizeof(GeomysPrefs);
	FSWrite(pb.ioParam.ioRefNum, &count, (Ptr)prefs);
	FSClose(pb.ioParam.ioRefNum);
}
