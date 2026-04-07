/*
 * dialogs.h - Dialog management for Geomys
 */

#ifndef DIALOGS_H
#define DIALOGS_H

void do_about(void);
void setup_default_button_outline(DialogPtr dlg, short outline_item);
void center_dialog_on_screen(DialogPtr dlg);
short modal_dialog_proc(void);
DialogPtr get_modal_dialog(short dlog_id);
pascal Boolean std_dlg_filter(DialogPtr dlg, EventRecord *evt, short *item);

/* Status window for connection progress */
WindowPtr conn_status_show(const char *msg);
void conn_status_update(WindowPtr w, const char *msg);
void conn_status_close(WindowPtr w);

#endif /* DIALOGS_H */
