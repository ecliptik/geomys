/*
 * history.h - Navigation history stack
 */

#ifndef HISTORY_H
#define HISTORY_H

#include <Types.h>

#define HISTORY_MAX  10

typedef struct {
	char    host[64];
	short   port;
	char    selector[256];
	char    type;
	char    title[80];
	char    query[256];  /* search query for type-7 entries */
} HistoryEntry;

/* Initialize history — call once at startup */
void history_init(void);

/* Push a new entry. Clears any forward entries.
 * query may be NULL for non-search entries. */
void history_push(const char *host, short port, char type,
    const char *selector, const char *title,
    const char *query);

/* Can go back? */
Boolean history_can_back(void);

/* Can go forward? */
Boolean history_can_forward(void);

/* Go back — returns pointer to previous entry, or NULL */
const HistoryEntry *history_back(void);

/* Go forward — returns pointer to next entry, or NULL */
const HistoryEntry *history_forward(void);

/* Get current entry (for refresh) */
const HistoryEntry *history_current(void);

/* Undo last back (move forward without checking) */
void history_undo_back(void);

/* Undo last forward (move back without checking) */
void history_undo_forward(void);

/* Get current position index (for cache coordination) */
short history_current_index(void);

#endif /* HISTORY_H */
