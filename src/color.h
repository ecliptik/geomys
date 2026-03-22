/*
 * color.h - Color QuickDraw detection for Geomys
 *
 * Detects Color QuickDraw at runtime via SysEnvirons().
 * On System 6 / Mac Plus, hasColorQD will be false and
 * all color code is skipped at zero cost.
 */

#ifndef COLOR_H
#define COLOR_H

#ifdef GEOMYS_COLOR
#include <Quickdraw.h>

/* Global flag: true if Color QuickDraw is available */
extern unsigned char g_has_color_qd;

/* Detect Color QuickDraw at startup via SysEnvirons() */
void color_detect(void);

#else
/* Without GEOMYS_COLOR: no color support */
#define g_has_color_qd  0
#define color_detect()  ((void)0)
#endif

#endif /* COLOR_H */
