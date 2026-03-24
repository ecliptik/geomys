/*
 * savefile.h - Save current page content to text file
 */

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "gopher.h"

#ifdef GEOMYS_DOWNLOAD
void do_save_page(void);
void do_download_file(const GopherItem *item);
void do_image_save(const GopherItem *item);
#else
#define do_save_page() ((void)0)
#define do_download_file(item) ((void)0)
#define do_image_save(item) ((void)0)
#endif

#endif /* SAVEFILE_H */
