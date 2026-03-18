/*
 * cache.h - Local page caching for instant back/forward
 *
 * Maintains a parallel cache array to the history stack.
 * Stores copies of page content (directory items or text buffer)
 * so back/forward navigation doesn't require re-fetching.
 *
 * Memory budget: ~100KB max (3 cached pages).
 */

#ifndef CACHE_H
#define CACHE_H

#ifdef GEOMYS_CACHE

#include "gopher.h"

/* Maximum cached pages (conservative for 4MB Mac Plus) */
#define CACHE_MAX  3

/* Initialize cache — call once at startup */
void cache_init(void);

/* Clean up all cache entries */
void cache_cleanup(void);

/* Store current page content into cache slot for history index.
 * Copies items array or text buffer from GopherState. */
void cache_store(short history_idx, const GopherState *gs);

/* Retrieve cached page content into GopherState.
 * Returns true if cache hit, false if miss. */
Boolean cache_retrieve(short history_idx, GopherState *gs);

/* Invalidate cache entry at history index (e.g., on refresh) */
void cache_invalidate(short history_idx);

/* Invalidate all entries from index onward (e.g., on new navigation) */
void cache_invalidate_from(short history_idx);

/* Get current history position (for cache coordination) */
short history_current_index(void);

#endif /* GEOMYS_CACHE */
#endif /* CACHE_H */
