# Shared GWorld Implementation Plan

Reduce multi-window memory footprint by sharing a single offscreen GWorld
across all browser windows instead of allocating one per window.

## Problem Statement

On System 7 with Color QuickDraw, each window currently gets its own GWorld
offscreen buffer. At 8-bit depth on a large display, each GWorld is ~400KB+.
With `GEOMYS_MAX_WINDOWS=3`, that's 1.2MB+ just for offscreen buffers — nearly
half the 2560KB preferred allocation in the SIZE resource.

## Current Architecture

### Offscreen State (src/offscreen.c)

All offscreen state is **module-static** (file-scoped globals), not per-session:

```c
/* GWorld color path */
static GWorldPtr g_color_gworld;    /* single GWorld (color) */
static CGrafPtr  g_saved_port;      /* saved port during begin/end */
static GDHandle  g_saved_gd;        /* saved device during begin/end */
static short     g_use_gworld;      /* 1 if using GWorld path */

/* Monochrome path */
static BitMap  g_offscreen;         /* offscreen bitmap descriptor */
static Ptr     g_offscreen_bits;    /* pixel data */
static BitMap  g_saved_bits;        /* saved real portBits */

/* Common */
static short   g_ready;             /* 1 if buffer allocated */
static short   g_active;            /* 1 if between begin/end */
```

Key observation: there is already only **one** GWorld/BitMap at the module
level. The state is NOT stored per-session in `BrowserSession`.

### Lifecycle

1. **`offscreen_init(win)`** — Called from `init_session()` for every new
   window (main.c:480). Creates the GWorld/BitMap sized to the window's
   `portRect`. On the color path, if a GWorld already exists, the code has
   an early-return (`if (g_offscreen_bits) return 1` on mono) but on the
   GWorld path it **always creates a new one** — potentially leaking the
   previous GWorld from another window.

2. **`offscreen_begin(win)`** — Called before content drawing. Checks if the
   buffer is large enough for the target window. If too small, silently skips
   offscreen rendering.

3. **`offscreen_end(win, blit_rect)`** — Blits from offscreen to screen.

4. **`offscreen_resize(win)`** — Called on window grow/zoom (main.c:2811,
   2855). Grows the buffer if the window exceeds current dimensions. Never
   shrinks.

5. **`offscreen_cleanup()`** — Called once at app exit (main.c:1194).

### Session Switching

`content_save_state` / `content_load_state` do **NOT** save any offscreen
state. The offscreen module is not session-aware. This means:

- When switching sessions, the existing GWorld remains in place.
- `offscreen_begin` checks the buffer covers the target window and
  silently skips if it doesn't.
- `offscreen_resize` grows the buffer if needed.

### The Actual Bug

The current code already behaves as a "shared" buffer at runtime because the
statics are singletons. But `offscreen_init()` is called for **every new
window**, and on the GWorld color path it **replaces** the existing GWorld
with one sized to the new window — potentially making it smaller than the
previous window needed. The mono path has an early-return guard
(`if (g_offscreen_bits) return 1`) but the GWorld path does not.

## New Architecture

### Goal

Make the "one GWorld shared across all windows" behavior **explicit and
correct**:

1. Initialize the offscreen buffer **once** at app startup.
2. Automatically grow it to the maximum of all window sizes.
3. Never re-create or replace it during new window creation.
4. Clean up once at app exit.

### Design Principles

- **Grow-only**: The shared GWorld grows to accommodate the largest window
  seen, but never shrinks. This is the existing behavior of
  `offscreen_resize()` and is correct — memory is not reclaimed until exit.
- **No per-window state**: No offscreen fields in `BrowserSession`. No
  save/restore during session switch. (Already the case.)
- **Cooperative multitasking**: Only one window draws at a time, so one
  buffer is sufficient. There is no concurrent access concern.
- **API unchanged**: `offscreen_begin/end/resize` keep the same signatures.
  Callers (content.c, main.c) need minimal changes.

## Implementation

### Phase 1: Fix offscreen_init to be idempotent (the core fix)

**File: src/offscreen.c**

Change `offscreen_init()` so the GWorld path has the same guard as the mono
path: if a GWorld already exists, check if it's large enough, grow if needed,
and return.

```c
short
offscreen_init(WindowPtr win)
{
#ifdef GEOMYS_COLOR
    if (g_has_color_qd) {
        GDHandle gd = GetMainDevice();
        if (gd && (*(*gd)->gdPMap)->pixelSize > 1) {
            /* If GWorld already exists, ensure it covers this window */
            if (g_color_gworld) {
                offscreen_resize(win);  /* grow if needed */
                return 1;
            }

            /* First init: create GWorld */
            Rect bounds = win->portRect;
            QDErr err = NewGWorld(&g_color_gworld, 0,
                &bounds, 0L, 0L, 0);
            if (err == noErr && g_color_gworld) {
                /* Clear to white ... (existing code) ... */
                g_use_gworld = 1;
                g_ready = 1;
                g_active = 0;
                return 1;
            }
            /* Fall through to mono */
        }
    }
#endif

    if (g_offscreen_bits) {
        /* Already allocated — grow if this window is larger */
        offscreen_resize(win);
        return 1;
    }

    /* ... existing mono allocation code ... */
}
```

This is the **entire fix for the memory leak**. With this change, the second
and third windows reuse the existing GWorld instead of replacing it.

### Phase 2: Move init out of per-window path (cleanup)

**File: src/main.c**

Currently `offscreen_init()` is called in `init_session()` which runs for
every new window. Move it to one-time app init and make per-window calls
just do a resize check.

**Option A (minimal change):** Leave `offscreen_init()` call in
`init_session()`. Phase 1's idempotent guard makes this safe — subsequent
calls just check size and return.

**Option B (cleaner):** Call `offscreen_init()` once in `main()` after
creating the first window, and replace the per-window call with
`offscreen_resize()`.

Recommendation: **Option A** for safety. The idempotent init is simple and
the call sites don't need to change. Option B can be done later as cleanup.

### Phase 3: Explicit grow-to-max in offscreen_resize (robustness)

**File: src/offscreen.c**

The current `offscreen_resize()` already implements grow-only semantics. No
changes needed to the resize logic. But add a comment documenting the shared
buffer contract:

```c
/*
 * offscreen_resize - Grow shared offscreen buffer if window exceeds it.
 *
 * The offscreen buffer is shared across all windows (cooperative
 * multitasking — only one window draws at a time). The buffer
 * grows to accommodate the largest window but never shrinks.
 * Called during window zoom/grow and from offscreen_init when a
 * new window is created.
 */
void
offscreen_resize(WindowPtr win)
```

### Phase 4: Update offscreen_begin size check (safety)

**File: src/offscreen.c**

`offscreen_begin()` already checks if the buffer covers the target window
and silently skips if too small. This is the correct fallback behavior.
Consider adding `offscreen_resize(win)` as an auto-grow attempt before
giving up:

```c
void
offscreen_begin(WindowPtr win)
{
    if (!g_ready || g_active)
        return;

#ifdef GEOMYS_COLOR
    if (g_use_gworld && g_color_gworld) {
        PixMapHandle pm;
        Rect gw_bounds;
        short win_w, win_h;

        pm = GetGWorldPixMap(g_color_gworld);
        gw_bounds = (*pm)->bounds;
        win_w = win->portRect.right - win->portRect.left;
        win_h = win->portRect.bottom - win->portRect.top;
        if (win_w > (gw_bounds.right - gw_bounds.left) ||
            win_h > (gw_bounds.bottom - gw_bounds.top)) {
            /* Try to grow before giving up */
            offscreen_resize(win);
            pm = GetGWorldPixMap(g_color_gworld);
            gw_bounds = (*pm)->bounds;
            if (win_w > (gw_bounds.right - gw_bounds.left) ||
                win_h > (gw_bounds.bottom - gw_bounds.top))
                return;  /* still too small, skip offscreen */
        }
        /* ... rest of begin ... */
    }
#endif
    /* Same pattern for mono path */
}
```

This makes the system more resilient to edge cases where resize was missed.

## Files to Modify

| File | Change | Phase |
|------|--------|-------|
| `src/offscreen.c` | Add idempotent guard to GWorld path in `offscreen_init()` | 1 |
| `src/offscreen.c` | Add resize-in-init for existing mono buffer | 1 |
| `src/offscreen.c` | Update header comment to document shared buffer design | 3 |
| `src/offscreen.c` | Optional: auto-grow in `offscreen_begin()` | 4 |
| `src/offscreen.h` | Update doc comment for `offscreen_init()` | 3 |

Files that do NOT need changes:
- `src/session.c` — No offscreen state in save/restore (already correct)
- `src/session.h` — No offscreen fields in BrowserSession (already correct)
- `src/content.c` — Calls offscreen_begin/end with WindowPtr (unchanged API)
- `src/main.c` — offscreen_init call in init_session is safe with idempotent guard
- `CMakeLists.txt` — No new flags or files needed

## Memory Savings

| Configuration | Before | After | Savings |
|---------------|--------|-------|---------|
| 1 window (System 7 color, 640x480 8-bit) | ~400KB | ~400KB | 0 |
| 2 windows (same size) | ~800KB | ~400KB | **~400KB** |
| 3 windows (same size) | ~1200KB | ~400KB | **~800KB** |
| 3 windows (varied sizes) | ~1200KB | ~400KB (largest) | **~800KB** |

On a 2560KB allocation, saving 800KB is a **31% reduction** in total memory
footprint, freeing space for more GopherItem arrays and page data.

## Testing Checklist

### Functional Tests

- [ ] Single window: offscreen rendering works identically to before
- [ ] Open 3 windows: all render with offscreen (no flicker)
- [ ] Resize window larger than initial: offscreen grows, rendering correct
- [ ] Zoom window: offscreen resize handles zoom correctly
- [ ] Close window, open new one: offscreen buffer reused
- [ ] Window cascade: different-sized windows share one buffer

### Memory Tests

- [ ] Open 3 windows on System 7 color: verify total heap usage is lower
- [ ] About This Macintosh shows reduced memory usage with 3 windows
- [ ] No "Connection failed" errors from heap exhaustion with 3 windows

### Regression Tests

- [ ] Monochrome path (System 6): no behavior change
- [ ] Color path: themes render correctly in all windows
- [ ] CopyBits blit: no garbage pixels, no wrong colors
- [ ] Session switching: no visual artifacts when switching focus
- [ ] Scrolling: offscreen blit correct during scroll in any window
- [ ] Back/forward navigation: content redraws correctly

### Edge Cases

- [ ] Create window, close it, create another: no leak, no crash
- [ ] Grow window to max screen size, then open small window: buffer stays large
- [ ] GWorld allocation failure (low memory): falls back to mono path

## Risk Assessment

**Risk: LOW**

This change is low risk because:

1. The offscreen buffer is already a singleton — there's only one GWorld
   in the module statics. The "per-window allocation" is actually a
   per-window *replacement*, which is the bug.

2. The fix is a 5-line guard at the top of `offscreen_init()` — minimal
   code change with maximum impact.

3. The API is unchanged — no callers need modification.

4. Cooperative multitasking guarantees only one window draws at a time,
   so sharing is inherently safe.

5. The mono path already has the correct idempotent guard (`if
   (g_offscreen_bits) return 1`). We're just applying the same pattern
   to the GWorld path.

**Potential issue:** If a window is closed that was the largest, the buffer
remains at the larger size. This is intentional (grow-only) and wastes at
most the difference between the largest and second-largest window. Not worth
the complexity of shrink logic.

## Implementation Order

1. Phase 1 (core fix) — 10 minutes, immediate memory savings
2. Phase 3 (documentation) — 5 minutes, clarity for maintainers
3. Phase 4 (auto-grow in begin) — 10 minutes, robustness improvement
4. Phase 2 Option B (cleanup) — optional, can defer indefinitely
