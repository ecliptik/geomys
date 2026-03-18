/*
 * dialogs.h - Dialog management for Geomys
 */

#ifndef DIALOGS_H
#define DIALOGS_H

void do_about(void);
void setup_default_button_outline(DialogPtr dlg, short outline_item);
pascal Boolean std_dlg_filter(DialogPtr dlg, EventRecord *evt, short *item);

#endif /* DIALOGS_H */
