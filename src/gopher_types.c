/*
 * gopher_types.c - Gopher item type registry
 * All 18 canonical and non-canonical types from RFC 1436
 */

#include <Types.h>
#include "gopher_types.h"
#include "gopher.h"

/* Download/image types are only navigable when GEOMYS_DOWNLOAD
 * is enabled — otherwise the click interceptors are compiled
 * out and gopher_navigate() would fall through to directory
 * parsing, producing garbage. */
#ifdef GEOMYS_DOWNLOAD
#define DL_NAV true
#else
#define DL_NAV false
#endif

#ifdef GEOMYS_HTML
#define HTML_NAV true
#else
#define HTML_NAV false
#endif

static const GopherTypeInfo type_table[] = {
	/* Canonical types (RFC 1436) */
	{ GOPHER_TEXT,      "TXT",  true   },
	{ GOPHER_DIRECTORY, "DIR",  true   },
	{ GOPHER_CSO,       "CSO",  true   },
	{ GOPHER_ERROR,     "ERR",  false  },
	{ GOPHER_BINHEX,    "HQX",  DL_NAV },
	{ GOPHER_DOS,       "DOS",  DL_NAV },
	{ GOPHER_UUENCODE,  "UUE",  DL_NAV },
	{ GOPHER_SEARCH,    "?  ",  true   },
	{ GOPHER_TELNET,    "TEL",  false  },
	{ GOPHER_BINARY,    "BIN",  DL_NAV },
	{ GOPHER_GIF,       "GIF",  DL_NAV },
	{ GOPHER_IMAGE,     "IMG",  DL_NAV },
	{ GOPHER_TN3270,    "3270", false  },

	/* Non-canonical types */
	{ GOPHER_DOC,       "DOC",  DL_NAV },
	{ GOPHER_HTML,      "HTM",  HTML_NAV },
	{ GOPHER_INFO,      "   ",  false  },
	{ GOPHER_PNG,       "PNG",  DL_NAV },
	{ GOPHER_RTF,       "RTF",  DL_NAV },
	{ GOPHER_SOUND,     "SND",  DL_NAV },

	{ 0, 0L, false }  /* sentinel */
};

static const GopherTypeInfo unknown_type = {
	'?', "???", false
};

const GopherTypeInfo *
gopher_type_info(char type)
{
	const GopherTypeInfo *p;

	for (p = type_table; p->type != 0; p++) {
		if (p->type == type)
			return p;
	}
	return &unknown_type;
}

const char *
gopher_type_label(char type)
{
	return gopher_type_info(type)->label;
}

Boolean
gopher_type_navigable(char type)
{
	return gopher_type_info(type)->navigable;
}

Boolean
gopher_type_is_download(char type)
{
	switch (type) {
	case GOPHER_BINHEX:
	case GOPHER_DOS:
	case GOPHER_UUENCODE:
	case GOPHER_BINARY:
	case GOPHER_DOC:
	case GOPHER_GIF:
	case GOPHER_IMAGE:
	case GOPHER_PNG:
	case GOPHER_RTF:
	case GOPHER_SOUND:
		return true;
	default:
		return false;
	}
}
