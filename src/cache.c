/*
 * cache.c - Local page caching for instant back/forward
 *
 * Uses a pool of up to CACHE_MAX slots, with the active count
 * determined at init based on available memory. Each slot stores
 * a copy of a page's content (directory items or text buffer).
 * Eviction uses weighted LRU: access_time + hit_count * 100,
 * so frequently-visited pages (e.g. home directory) stay cached.
 *
 * On System 7+, large allocations use TempNewHandle to draw from
 * temporary (MultiFinder) memory, keeping the app heap free for
 * working data. Falls back to NewPtr on System 6 or if temp
 * memory is unavailable.
 *
 * Memory budget: ~50KB per slot worst case.
 */

#ifdef GEOMYS_CACHE

#include <Memory.h>
#include <Gestalt.h>
#include <string.h>

#include "cache.h"
#include "gopher.h"
#include "sysutil.h"

typedef struct {
	short       session_id;     /* owning session, -1 = empty */
	short       history_idx;    /* which history entry this caches */
	short       page_type;      /* PAGE_DIRECTORY or PAGE_TEXT */
	GopherItem  *items;         /* copied items array */
	short       item_count;
	char        *text_buf;      /* copied text buffer */
	long        text_len;
	long        *text_lines;    /* copied line index */
	short       text_line_count;
	long        access_time;    /* tick count for LRU */
	short       hit_count;      /* times retrieved (for weighted eviction) */
	/* Handle tracking for TempNewHandle allocations.
	 * Non-NULL means the pointer came from temp memory
	 * and must be freed with DisposeHandle. */
	Handle      items_h;
	Handle      text_buf_h;
	Handle      text_lines_h;
} CacheSlot;

static Boolean g_has_temp_mem = false;  /* System 7+ temp memory available */

/*
 * cache_alloc - Allocate memory, preferring temporary memory.
 * Tries TempNewHandle first (System 7 MultiFinder temp memory),
 * locks the handle, and returns the dereferenced pointer.
 * Falls back to NewPtr if temp memory is unavailable.
 * Sets *out_h to the Handle if temp memory was used, else NULL.
 */
static Ptr
cache_alloc(long size, Handle *out_h)
{
	*out_h = 0L;

	/* Try temporary memory first (System 7+ only) */
	if (g_has_temp_mem) {
		OSErr err;
		Handle h;

		h = TempNewHandle(size, &err);
		if (h && err == noErr) {
			MoveHHi(h);
			HLock(h);
			*out_h = h;
			return *h;
		}
	}

	/* Fall back to app heap */
	return NewPtr(size);
}

/*
 * cache_free - Free a pointer allocated by cache_alloc.
 * Uses DisposeHandle if h is non-NULL, else DisposePtr.
 */
static void
cache_free(Ptr p, Handle h)
{
	if (h)
		DisposeHandle(h);
	else if (p)
		DisposePtr(p);
}

static CacheSlot g_cache[CACHE_MAX];
static long g_tick = 0;       /* monotonic counter for LRU */
static short g_cache_limit;   /* active slots (may be < CACHE_MAX) */

static void
free_slot(CacheSlot *slot)
{
	if (slot->items) {
		cache_free((Ptr)slot->items, slot->items_h);
		slot->items = 0L;
		slot->items_h = 0L;
	}
	if (slot->text_buf) {
		cache_free(slot->text_buf, slot->text_buf_h);
		slot->text_buf = 0L;
		slot->text_buf_h = 0L;
	}
	if (slot->text_lines) {
		cache_free((Ptr)slot->text_lines, slot->text_lines_h);
		slot->text_lines = 0L;
		slot->text_lines_h = 0L;
	}
	slot->session_id = -1;
	slot->history_idx = -1;
	slot->page_type = PAGE_NONE;
	slot->item_count = 0;
	slot->text_len = 0;
	slot->text_line_count = 0;
	slot->hit_count = 0;
}

/* Find slot for a session + history index, or -1 */
static short
find_slot(short session_id, short history_idx)
{
	short i;

	for (i = 0; i < g_cache_limit; i++) {
		if (g_cache[i].session_id == session_id &&
		    g_cache[i].history_idx == history_idx)
			return i;
	}
	return -1;
}

/* Find best eviction candidate using weighted LRU.
 * Score = access_time + hit_count * 100. Lower = evict first.
 * Empty slots are returned immediately. */
static short
find_evict_slot(void)
{
	short i, best = 0;
	long best_score;

	if (g_cache[0].session_id == -1)
		return 0;

	best_score = g_cache[0].access_time +
	    (long)g_cache[0].hit_count * 100;

	for (i = 1; i < g_cache_limit; i++) {
		long score;

		if (g_cache[i].session_id == -1)
			return i;  /* empty slot, use it */

		score = g_cache[i].access_time +
		    (long)g_cache[i].hit_count * 100;
		if (score < best_score) {
			best_score = score;
			best = i;
		}
	}
	return best;
}

void
cache_init(void)
{
	short i;
	long free_mem;

	/* Check for System 7+ temporary memory support */
	g_has_temp_mem = false;
	if (TrapAvailable(_GestaltDispatch)) {
		long resp;

		if (Gestalt(gestaltSystemVersion, &resp) == noErr &&
		    resp >= 0x0700)
			g_has_temp_mem = true;
	}

	/* Determine how many slots to use based on
	 * available memory. Reserve 768KB for the app
	 * (code + stack + working set) and use the rest
	 * to scale cache. ~50KB per slot worst case. */
	free_mem = FreeMem();
	g_cache_limit = 3;  /* minimum */
	if (free_mem > 768L * 1024L) {
		long budget = free_mem - 768L * 1024L;
		short extra = (short)(budget / (50L * 1024L));
		g_cache_limit = 3 + extra;
	}
	if (g_cache_limit > CACHE_MAX)
		g_cache_limit = CACHE_MAX;

	for (i = 0; i < CACHE_MAX; i++) {
		g_cache[i].session_id = -1;
		g_cache[i].history_idx = -1;
		g_cache[i].items = 0L;
		g_cache[i].text_buf = 0L;
		g_cache[i].text_lines = 0L;
		g_cache[i].items_h = 0L;
		g_cache[i].text_buf_h = 0L;
		g_cache[i].text_lines_h = 0L;
		g_cache[i].page_type = PAGE_NONE;
		g_cache[i].item_count = 0;
		g_cache[i].text_len = 0;
		g_cache[i].text_line_count = 0;
		g_cache[i].access_time = 0;
		g_cache[i].hit_count = 0;
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
cache_store(short session_id, short history_idx, const GopherState *gs)
{
	short slot_idx;
	CacheSlot *slot;

	if (!gs || gs->page_type == PAGE_NONE)
		return;

	/* Skip caching when memory is low — prevent cache
	 * from consuming the last available memory */
	if (FreeMem() < 200L * 1024L)
		return;

	/* Reuse existing slot or evict */
	slot_idx = find_slot(session_id, history_idx);
	if (slot_idx < 0) {
		slot_idx = find_evict_slot();
		free_slot(&g_cache[slot_idx]);
	}

	slot = &g_cache[slot_idx];
	slot->session_id = session_id;
	slot->history_idx = history_idx;
	slot->page_type = gs->page_type;
	slot->access_time = ++g_tick;
	slot->hit_count = 0;

	if (gs->page_type == PAGE_DIRECTORY && gs->items &&
	    gs->item_count > 0) {
		long size = (long)gs->item_count * sizeof(GopherItem);

		slot->items = (GopherItem *)cache_alloc(size,
		    &slot->items_h);
		if (!slot->items) {
			/* Allocation failed — try evicting
			 * another slot to free memory */
			short victim = find_evict_slot();
			if (victim != slot_idx) {
				free_slot(&g_cache[victim]);
				slot->items = (GopherItem *)
				    cache_alloc(size,
				    &slot->items_h);
			}
		}
		if (slot->items) {
			memcpy(slot->items, gs->items, size);
			slot->item_count = gs->item_count;
		} else {
			/* Still failed — invalidate */
			free_slot(slot);
			return;
		}
	}

	if ((gs->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || gs->page_type == PAGE_HTML
#endif
	    ) && gs->text_buf && gs->text_len > 0) {
		slot->text_buf = cache_alloc(gs->text_len,
		    &slot->text_buf_h);
		if (!slot->text_buf) {
			/* Try evicting another slot */
			short victim = find_evict_slot();
			if (victim != slot_idx) {
				free_slot(&g_cache[victim]);
				slot->text_buf = cache_alloc(
				    gs->text_len,
				    &slot->text_buf_h);
			}
		}
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

			slot->text_lines = (long *)cache_alloc(
			    lsize, &slot->text_lines_h);
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
cache_retrieve(short session_id, short history_idx, GopherState *gs)
{
	short slot_idx;
	CacheSlot *slot;

	slot_idx = find_slot(session_id, history_idx);
	if (slot_idx < 0)
		return false;

	slot = &g_cache[slot_idx];
	slot->access_time = ++g_tick;
	slot->hit_count++;

	gs->page_type = slot->page_type;

	if (slot->page_type == PAGE_DIRECTORY && slot->items) {
		long size = (long)slot->item_count * sizeof(GopherItem);
		/* Allocate exactly what we need, rounded up
		 * to GOPHER_INIT_ITEMS minimum */
		short need = slot->item_count;
		long buf_size;

		if (need < GOPHER_INIT_ITEMS)
			need = GOPHER_INIT_ITEMS;
		buf_size = (long)need * sizeof(GopherItem);

		if (!gs->items ||
		    gs->item_capacity < slot->item_count) {
			if (gs->items)
				DisposePtr((Ptr)gs->items);
			gs->items = (GopherItem *)NewPtr(
			    buf_size);
			if (!gs->items) {
				gs->item_capacity = 0;
				return false;
			}
			gs->item_capacity = need;
		}
		/* Clear buffer then copy cached items */
		memset(gs->items, 0,
		    (long)gs->item_capacity *
		    sizeof(GopherItem));
		memcpy(gs->items, slot->items, size);
		gs->item_count = slot->item_count;

		/* Reset text fields — directory page has no text */
		gs->text_len = 0;
		gs->text_line_count = 0;
		return true;
	}

	if ((slot->page_type == PAGE_TEXT
#ifdef GEOMYS_HTML
	    || slot->page_type == PAGE_HTML
#endif
	    ) && slot->text_buf) {
		if (!gs->text_buf ||
		    gs->text_buf_capacity < slot->text_len + 1) {
			/* Allocate with 1KB headroom, clamped to
			 * [GOPHER_TEXT_INIT_SIZE, GOPHER_TEXT_BUFSIZ].
			 * Avoids wasting 30KB on small pages. */
			long alloc_size = slot->text_len + 1024;

			if (alloc_size < GOPHER_TEXT_INIT_SIZE)
				alloc_size = GOPHER_TEXT_INIT_SIZE;
			if (alloc_size > GOPHER_TEXT_BUFSIZ)
				alloc_size = GOPHER_TEXT_BUFSIZ;
			if (gs->text_buf)
				DisposePtr(gs->text_buf);
			gs->text_buf = NewPtr(alloc_size);
			if (!gs->text_buf) {
				gs->text_buf_capacity = 0;
				return false;
			}
			gs->text_buf_capacity = alloc_size;
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
				short need = slot->text_line_count * 2;
				if (need < GOPHER_INIT_TEXT_LINES)
					need = GOPHER_INIT_TEXT_LINES;
				if (need > GOPHER_MAX_TEXT_LINES)
					need = GOPHER_MAX_TEXT_LINES;
				gs->text_lines = (long *)NewPtr(
				    (long)need * sizeof(long));
				gs->text_lines_capacity = need;
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
				short need = GOPHER_INIT_TEXT_LINES;
				if (need > GOPHER_MAX_TEXT_LINES)
					need = GOPHER_MAX_TEXT_LINES;
				gs->text_lines = (long *)NewPtr(
				    (long)need * sizeof(long));
				gs->text_lines_capacity = need;
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
cache_invalidate(short session_id, short history_idx)
{
	short slot_idx;

	slot_idx = find_slot(session_id, history_idx);
	if (slot_idx >= 0)
		free_slot(&g_cache[slot_idx]);
}

void
cache_invalidate_from(short session_id, short history_idx)
{
	short i;

	for (i = 0; i < g_cache_limit; i++) {
		if (g_cache[i].session_id == session_id &&
		    g_cache[i].history_idx >= history_idx)
			free_slot(&g_cache[i]);
	}
}

#endif /* GEOMYS_CACHE */
