/*
 * history.c - Navigation history stack
 */

#include <string.h>
#include "history.h"

static HistoryEntry g_history[HISTORY_MAX];
static short g_count = 0;    /* total entries */
static short g_pos = -1;     /* current position (-1 = none) */

void
history_init(void)
{
	g_count = 0;
	g_pos = -1;
}

void
history_push(const char *host, short port, char type,
    const char *selector, const char *title,
    const char *query)
{
	HistoryEntry *e;

	/* Clear forward entries when navigating from mid-stack */
	if (g_pos < g_count - 1)
		g_count = g_pos + 1;

	/* If stack is full, shift everything left */
	if (g_count >= HISTORY_MAX) {
		short i;
		for (i = 1; i < HISTORY_MAX; i++)
			g_history[i - 1] = g_history[i];
		g_count = HISTORY_MAX - 1;
	}

	e = &g_history[g_count];
	strncpy(e->host, host, sizeof(e->host) - 1);
	e->host[sizeof(e->host) - 1] = '\0';
	e->port = port;
	e->type = type;
	strncpy(e->selector, selector, sizeof(e->selector) - 1);
	e->selector[sizeof(e->selector) - 1] = '\0';
	strncpy(e->title, title, sizeof(e->title) - 1);
	e->title[sizeof(e->title) - 1] = '\0';

	if (query) {
		strncpy(e->query, query, sizeof(e->query) - 1);
		e->query[sizeof(e->query) - 1] = '\0';
	} else {
		e->query[0] = '\0';
	}
	e->scroll_pos = 0;

	g_pos = g_count;
	g_count++;
}

Boolean
history_can_back(void)
{
	return g_pos > 0;
}

Boolean
history_can_forward(void)
{
	return g_pos < g_count - 1;
}

const HistoryEntry *
history_back(void)
{
	if (!history_can_back())
		return 0L;
	g_pos--;
	return &g_history[g_pos];
}

const HistoryEntry *
history_forward(void)
{
	if (!history_can_forward())
		return 0L;
	g_pos++;
	return &g_history[g_pos];
}

const HistoryEntry *
history_current(void)
{
	if (g_pos < 0 || g_pos >= g_count)
		return 0L;
	return &g_history[g_pos];
}

void
history_undo_back(void)
{
	if (g_pos < g_count - 1)
		g_pos++;
}

void
history_undo_forward(void)
{
	if (g_pos > 0)
		g_pos--;
}

short
history_current_index(void)
{
	return g_pos;
}

void
history_set_scroll(short scroll_pos)
{
	if (g_pos >= 0 && g_pos < g_count)
		g_history[g_pos].scroll_pos = scroll_pos;
}

short
history_get_scroll(const HistoryEntry *e)
{
	return e->scroll_pos;
}
