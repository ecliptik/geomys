/*
 * cache.h - Local page caching for instant back/forward
 *
 * Maintains a parallel cache array to the history stack.
 * Stores copies of page content (directory items or text buffer)
 * so back/forward navigation doesn't require re-fetching.
 *
 * Active slot count is determined at init based on FreeMem(),
 * up to CACHE_MAX ceiling. Eviction uses weighted LRU
 * (access_time + hit_count * 100) so frequently-visited
 * pages stay cached longer.
 */

#ifndef CACHE_H
#define CACHE_H

#ifdef GEOMYS_CACHE

#include "gopher.h"

/* Maximum cached pages (conservative for 4MB Mac Plus).
 * Can be overridden via CMake compile definition for multi-window builds. */
#ifndef CACHE_MAX
#define CACHE_MAX  3
#endif

/* Initialize cache — call once at startup */
void cache_init(void);

/* Clean up all cache entries */
void cache_cleanup(void);

/* Store current page content into cache slot.
 * session_id scopes the entry to avoid cross-window collisions.
 * Copies items array or text buffer from GopherState. */
void cache_store(short session_id, short history_idx,
    const GopherState *gs);

/* Retrieve cached page content into GopherState.
 * Returns true if cache hit, false if miss. */
Boolean cache_retrieve(short session_id, short history_idx,
    GopherState *gs);

/* Invalidate cache entry at history index (e.g., on refresh) */
void cache_invalidate(short session_id, short history_idx);

/* Invalidate all entries from index onward (e.g., on new navigation) */
void cache_invalidate_from(short session_id, short history_idx);

/* Get current history position (for cache coordination) */
short history_current_index(void);

#endif /* GEOMYS_CACHE */
#endif /* CACHE_H */
