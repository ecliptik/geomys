/*
 * color.c - Color QuickDraw detection for Geomys
 *
 * Uses Gestalt(gestaltQuickdrawVersion) when available (System 6.0.4+),
 * falls back to SysEnvirons() on older systems without Gestalt.
 * On System 6 / Mac Plus, hasColorQD will be false.
 */

#ifdef GEOMYS_COLOR

#include <OSUtils.h>
#include <Gestalt.h>
#include <Quickdraw.h>
#include <Multiverse.h>
#include "color.h"
#include "sysutil.h"

/* gestalt8BitQD may not be defined in Retro68 headers */
#ifndef gestalt8BitQD
#define gestalt8BitQD 0x0100
#endif

unsigned char g_has_color_qd = 0;

void
color_detect(void)
{
	long resp;

	g_has_color_qd = 0;

	/* Prefer Gestalt — available on System 6.0.4+ */
	if (TrapAvailable(_GestaltDispatch) &&
	    Gestalt(gestaltQuickdrawVersion, &resp) == noErr &&
	    resp >= gestalt8BitQD) {
		g_has_color_qd = 1;
		return;
	}

	/* Fallback for System 6 without Gestalt */
	{
		SysEnvRec env;

		if (SysEnvirons(1, &env) == noErr && env.hasColorQD)
			g_has_color_qd = 1;
	}
}

#endif /* GEOMYS_COLOR */
