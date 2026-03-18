/*
 * gopher_types.c - Gopher item type registry
 * All 18 canonical and non-canonical types from RFC 1436
 */

#include <Types.h>
#include "gopher_types.h"
#include "gopher.h"

static const GopherTypeInfo type_table[] = {
	/* Canonical types (RFC 1436) */
	{ GOPHER_TEXT,      "TXT",  true  },
	{ GOPHER_DIRECTORY, "DIR",  true  },
	{ GOPHER_CSO,       "CSO",  true  },
	{ GOPHER_ERROR,     "ERR",  false },
	{ GOPHER_BINHEX,    "HQX",  false },
	{ GOPHER_DOS,       "DOS",  false },
	{ GOPHER_UUENCODE,  "UUE",  false },
	{ GOPHER_SEARCH,    "?",    true  },
	{ GOPHER_TELNET,    "TEL",  false },
	{ GOPHER_BINARY,    "BIN",  false },
	{ GOPHER_GIF,       "GIF",  false },
	{ GOPHER_IMAGE,     "IMG",  false },
	{ GOPHER_TN3270,    "3270", false },

	/* Non-canonical types */
	{ GOPHER_DOC,       "DOC",  false },
	{ GOPHER_HTML,      "HTM",  true  },
	{ GOPHER_INFO,      "   ",  false },
	{ GOPHER_PNG,       "PNG",  false },
	{ GOPHER_RTF,       "RTF",  false },
	{ GOPHER_SOUND,     "SND",  false },

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
