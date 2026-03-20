/*
 * cache.c - Local page caching for instant back/forward
 *
 * Uses a fixed pool of CACHE_MAX slots. Each slot stores a copy
 * of a page's content (directory items or text buffer). LRU
 * eviction when all slots are full.
 *
 * Total memory: ~100KB worst case (3 slots x ~33KB each)
 */

#ifdef GEOMYS_CACHE

#include <Memory.h>
#include <string.h>

#include "cache.h"
#include "gopher.h"

typedef struct {
	short       history_idx;    /* which history entry this caches, -1 = empty */
	short       page_type;      /* PAGE_DIRECTORY or PAGE_TEXT */
	GopherItem  *items;         /* copied items array (NewPtr) */
	short       item_count;
	char        *text_buf;      /* copied text buffer (NewPtr) */
	long        text_len;
	long        *text_lines;    /* copied line index (NewPtr) */
	short       text_line_count;
	long        access_time;    /* tick count for LRU */
} CacheSlot;

static CacheSlot g_cache[CACHE_MAX];
static long g_tick = 0;  /* simple monotonic counter for LRU */

static void
free_slot(CacheSlot *slot)
{
	if (slot->items) {
		DisposePtr((Ptr)slot->items);
		slot->items = 0L;
	}
	if (slot->text_buf) {
		DisposePtr(slot->text_buf);
		slot->text_buf = 0L;
	}
	if (slot->text_lines) {
		DisposePtr((Ptr)slot->text_lines);
		slot->text_lines = 0L;
	}
	slot->history_idx = -1;
	slot->page_type = PAGE_NONE;
	slot->item_count = 0;
	slot->text_len = 0;
	slot->text_line_count = 0;
}

/* Find slot for a history index, or -1 */
static short
find_slot(short history_idx)
{
	short i;

	for (i = 0; i < CACHE_MAX; i++) {
		if (g_cache[i].history_idx == history_idx)
			return i;
	}
	return -1;
}

/* Find LRU slot (oldest access_time) */
static short
find_lru_slot(void)
{
	short i, lru = 0;
	long oldest = g_cache[0].access_time;

	for (i = 1; i < CACHE_MAX; i++) {
		if (g_cache[i].history_idx == -1)
			return i;  /* empty slot, use it */
		if (g_cache[i].access_time < oldest) {
			oldest = g_cache[i].access_time;
			lru = i;
		}
	}
	return lru;
}

void
cache_init(void)
{
	short i;

	for (i = 0; i < CACHE_MAX; i++) {
		g_cache[i].history_idx = -1;
		g_cache[i].items = 0L;
		g_cache[i].text_buf = 0L;
		g_cache[i].text_lines = 0L;
		g_cache[i].page_type = PAGE_NONE;
		g_cache[i].item_count = 0;
		g_cache[i].text_len = 0;
		g_cache[i].text_line_count = 0;
		g_cache[i].access_time = 0;
	}
	g_tick = 0;
}

void
cache_cleanup(void)
{
	short i;

	for (i = 0; i < CACHE_MAX; i++)
		free_slot(&g_cache[i]);
}

void
cache_store(short history_idx, const GopherState *gs)
{
	short slot_idx;
	CacheSlot *slot;

	if (!gs || gs->page_type == PAGE_NONE)
		return;

	/* Reuse existing slot or evict LRU */
	slot_idx = find_slot(history_idx);
	if (slot_idx < 0) {
		slot_idx = find_lru_slot();
		free_slot(&g_cache[slot_idx]);
	}

	slot = &g_cache[slot_idx];
	slot->history_idx = history_idx;
	slot->page_type = gs->page_type;
	slot->access_time = ++g_tick;

	if (gs->page_type == PAGE_DIRECTORY && gs->items &&
	    gs->item_count > 0) {
		long size = (long)gs->item_count * sizeof(GopherItem);

		slot->items = (GopherItem *)NewPtr(size);
		if (slot->items) {
			memcpy(slot->items, gs->items, size);
			slot->item_count = gs->item_count;
		} else {
			/* Allocation failed — invalidate */
			free_slot(slot);
			return;
		}
	}

	if (gs->page_type == PAGE_TEXT && gs->text_buf &&
	    gs->text_len > 0) {
		slot->text_buf = NewPtr(gs->text_len);
		if (slot->text_buf) {
			memcpy(slot->text_buf, gs->text_buf,
			    gs->text_len);
			slot->text_len = gs->text_len;
		} else {
			free_slot(slot);
			return;
		}

		/* Store line index */
		if (gs->text_lines && gs->text_line_count > 0) {
			long lsize = (long)gs->text_line_count
			    * sizeof(long);

			slot->text_lines = (long *)NewPtr(lsize);
			if (slot->text_lines) {
				memcpy(slot->text_lines,
				    gs->text_lines, lsize);
				slot->text_line_count =
				    gs->text_line_count;
			}
			/* Non-fatal if line index alloc fails —
			 * it will be rebuilt on restore */
		}
	}
}

Boolean
cache_retrieve(short history_idx, GopherState *gs)
{
	short slot_idx;
	CacheSlot *slot;

	slot_idx = find_slot(history_idx);
	if (slot_idx < 0)
		return false;

	slot = &g_cache[slot_idx];
	slot->access_time = ++g_tick;

	gs->page_type = slot->page_type;

	if (slot->page_type == PAGE_DIRECTORY && slot->items) {
		long size = (long)slot->item_count * sizeof(GopherItem);
		long buf_size = (long)GOPHER_MAX_ITEMS * sizeof(GopherItem);

		if (!gs->items) {
			gs->items = (GopherItem *)NewPtr(buf_size);
			if (!gs->items)
				return false;
		}
		/* Clear entire buffer first to prevent stale items
		 * from previous page bleeding through on scroll */
		memset(gs->items, 0, buf_size);
		memcpy(gs->items, slot->items, size);
		gs->item_count = slot->item_count;

		/* Reset text fields — directory page has no text */
		gs->text_len = 0;
		gs->text_line_count = 0;
		return true;
	}

	if (slot->page_type == PAGE_TEXT && slot->text_buf) {
		if (!gs->text_buf) {
			gs->text_buf = NewPtr(GOPHER_TEXT_BUFSIZ);
			if (!gs->text_buf)
				return false;
		}
		memcpy(gs->text_buf, slot->text_buf,
		    slot->text_len);
		gs->text_len = slot->text_len;

		/* Reset directory fields — text page has no items */
		gs->item_count = 0;

		/* Restore line index */
		if (slot->text_lines &&
		    slot->text_line_count > 0) {
			if (!gs->text_lines) {
				gs->text_lines = (long *)NewPtr(
				    (long)GOPHER_MAX_TEXT_LINES
				    * sizeof(long));
			}
			if (gs->text_lines) {
				long lsize =
				    (long)slot->text_line_count
				    * sizeof(long);
				memcpy(gs->text_lines,
				    slot->text_lines, lsize);
				gs->text_line_count =
				    slot->text_line_count;
			}
		}

		/* Rebuild line index if not cached or
		 * allocation failed during store */
		if (gs->text_line_count == 0 &&
		    gs->text_len > 0) {
			if (!gs->text_lines) {
				gs->text_lines = (long *)NewPtr(
				    (long)GOPHER_MAX_TEXT_LINES
				    * sizeof(long));
			}
			if (gs->text_lines) {
				long ti;

				gs->text_lines[0] = 0;
				gs->text_line_count = 1;
				for (ti = 0; ti < gs->text_len;
				    ti++) {
					if (gs->text_buf[ti] ==
					    '\r' &&
					    gs->text_line_count <
					    GOPHER_MAX_TEXT_LINES) {
						gs->text_lines[
						    gs->text_line_count]
						    = ti + 1;
						gs->text_line_count++;
					}
				}
			}
		}
		return true;
	}

	return false;
}

void
cache_invalidate(short history_idx)
{
	short slot_idx;

	slot_idx = find_slot(history_idx);
	if (slot_idx >= 0)
		free_slot(&g_cache[slot_idx]);
}

void
cache_invalidate_from(short history_idx)
{
	short i;

	for (i = 0; i < CACHE_MAX; i++) {
		if (g_cache[i].history_idx >= history_idx)
			free_slot(&g_cache[i]);
	}
}

#endif /* GEOMYS_CACHE */
