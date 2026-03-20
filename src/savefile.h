/*
 * savefile.h - Save current page content to text file
 */

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "gopher.h"

#ifdef GEOMYS_DOWNLOAD
void do_save_page(void);
#else
#define do_save_page() ((void)0)
#endif

#endif /* SAVEFILE_H */
