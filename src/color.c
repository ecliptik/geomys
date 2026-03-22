/*
 * color.c - Color QuickDraw detection for Geomys
 *
 * Uses SysEnvirons() which is available on all Macs from 128K ROMs.
 * On System 6 / Mac Plus, hasColorQD will be false.
 */

#ifdef GEOMYS_COLOR

#include <OSUtils.h>
#include <Quickdraw.h>
#include <Multiverse.h>
#include "color.h"

unsigned char g_has_color_qd = 0;

void
color_detect(void)
{
	SysEnvRec env;

	g_has_color_qd = 0;
	if (SysEnvirons(1, &env) == noErr && env.hasColorQD)
		g_has_color_qd = 1;
}

#endif /* GEOMYS_COLOR */
