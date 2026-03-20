# Multi-Window Implementation Plan for Geomys

**Status:** Implemented (merged to main)
**Target:** Geomys v0.3.0
**Constraint:** 68000 CPU, 4 MiB RAM (Mac Plus), System 6.0.8, MacTCP
**Reference:** Flynn's `session.c/session.h` multi-session pattern

---

## Overview

This plan adds multi-window browsing to Geomys, allowing up to 4 simultaneous
Gopher windows on the 512x342 Mac Plus screen. The implementation follows
Flynn's proven session management pattern: a `BrowserSession` struct holds all
per-window state, managed via a static pointer array with save/restore state
swapping to minimize API churn.

The work is split into 6 phases. Each phase produces a working, testable build.
Earlier phases preserve single-window behavior exactly; multi-window behavior
emerges in Phases 2-3 and is polished in Phases 4-6.

---

## Phase 1: Foundation — BrowserSession Struct & State Migration

**Goal:** Introduce the `BrowserSession` struct and move all per-window global/static
state into it, while preserving identical single-window behavior. After this phase,
Geomys works exactly as before but all state flows through a session struct.

### Files to Create

- **`src/session.h`** — BrowserSession struct definition, session API declarations
- **`src/session.c`** — Session lifecycle (create, destroy, lookup), save/restore

### Files to Modify

- **`src/main.h`** — Add `extern BrowserSession *active_session;` declaration,
  add `GEOMYS_MAX_WINDOWS` default define
- **`src/main.c`** — Replace globals (`g_window`, `g_gopher`, `g_app_state`,
  `g_pending_scroll`) with `active_session->` access; update `main()` to create
  first session via `session_new()`; update event loop to use `active_session`
- **`src/browser.c`** — Add `browser_save_state()`/`browser_load_state()` functions
  that copy module statics (`g_addr_te`, `g_status`, `g_btn_state`, `g_btn_rects`,
  `g_addr_rect`, `g_focus`) to/from BrowserSession fields
- **`src/browser.h`** — Declare save/load state functions
- **`src/content.c`** — Add `content_save_state()`/`content_load_state()` functions
  for statics (`g_scrollbar`, `g_scroll_pos`, `g_page`, `g_hover_row`,
  `g_row_height`, `g_font_id`, `g_font_size`, `g_sel`, `g_win_active`)
- **`src/content.h`** — Declare save/load state functions
- **`src/history.c`** — Add `history_save_state()`/`history_load_state()` for
  statics (`g_history`, `g_count`, `g_pos`); simplest migration, do this first
- **`src/history.h`** — Declare save/load state functions
- **`CMakeLists.txt`** — Add `GEOMYS_MAX_WINDOWS` compile definition (default 1),
  conditionally add `src/session.c`

### Key Code Changes

**BrowserSession struct** (~12.5 KB per instance):
```c
typedef struct BrowserSession {
    WindowPtr       window;
    ControlHandle   scrollbar;
    GopherState     gopher;                    /* ~5.4 KB */
    HistoryEntry    history[HISTORY_MAX];       /* 10 x 661 = 6,610 bytes */
    short           history_count;
    short           history_pos;
    TEHandle        addr_te;
    char            status[80];
    short           btn_state[NAV_BTN_COUNT];
    Rect            btn_rects[NAV_BTN_COUNT];
    Rect            addr_rect;
    short           focus;
    short           scroll_pos;
    short           hover_row;
    short           row_height;
    short           font_id;
    short           font_size;
#ifdef GEOMYS_CLIPBOARD
    Selection       sel;
    short           win_active;
#endif
    short           app_state;
    short           pending_scroll;
    short           id;
} BrowserSession;
```

**Session array:**
```c
static BrowserSession *sessions[GEOMYS_MAX_WINDOWS];
static short num_sessions = 0;
BrowserSession *active_session = NULL;
```

**Save/restore pattern** (example for history.c):
```c
void history_save_state(BrowserSession *s) {
    memcpy(s->history, g_history, sizeof(g_history));
    s->history_count = g_count;
    s->history_pos = g_pos;
}
void history_load_state(BrowserSession *s) {
    memcpy(g_history, s->history, sizeof(g_history));
    g_count = s->history_count;
    g_pos = s->history_pos;
}
```

**Single-session fast path** (GEOMYS_MAX_WINDOWS == 1):
```c
#if GEOMYS_MAX_WINDOWS == 1
/* Zero overhead: static session, no save/restore needed */
static BrowserSession g_session;
#define active_session  (&g_session)
#endif
```

### Migration Order (within Phase 1)

1. Create `session.h` with struct definition
2. Add save/load to `history.c` (simplest: 3 statics)
3. Add save/load to `browser.c` (6 statics)
4. Add save/load to `content.c` (8+ statics)
5. Create `session.c` with `session_new()`/`session_destroy()`
6. Update `main.c` to create session and wire `active_session`

### Testing Checkpoint

- Build and run with `GEOMYS_MAX_WINDOWS=1` (default)
- Behavior must be **identical** to pre-refactor: navigate, scroll, back/forward,
  address bar editing, favorites, all work exactly as before
- Verify no memory leaks: About This Macintosh should show same usage
- Verify cache still works (back/forward instant restore)
- Verify clipboard/selection still works

### Risk Areas

- **History copy cost**: `memcpy` of 6.6 KB history on every save/restore.
  Mitigation: only save/restore history on actual session switch, not every tick.
  For Phase 1 with single session, this code path is never hit.
- **Content `g_page` pointer**: Currently points to `g_gopher`'s data. After
  migration, must point to `active_session->gopher`. Must ensure
  `content_set_page()` is called after `session_load_state()`.
- **TextEdit handle ownership**: Each session creates its own TEHandle in
  `browser_init()`. Must ensure TEHandle is properly saved/loaded (it's a handle,
  not data — save the handle value, don't copy TE data).

---

## Phase 2: Window Lifecycle — New Window & Close Window

**Goal:** Implement creating and destroying browser windows. After this phase,
the user can open additional windows and close them.

### Files to Modify

- **`src/session.c`** — Implement full `session_new()` with cascading window
  positions, scrollbar creation, TextEdit creation, gopher/history init;
  implement `session_destroy()` with connection close, control/TE disposal,
  window disposal, slot cleanup; implement `session_from_window()` lookup;
  implement `session_destroy_and_fixup()` for active session handling
- **`src/main.c`** — Add `do_new_window()` function that calls `session_new()`,
  navigates to home page; update `inGoAway` handler to call
  `session_destroy_and_fixup()`; handle last-window-closes-quits behavior
- **`src/main.h`** — Add `do_new_window()` declaration, `WINDOW_MENU_ID` define
- **`src/menus.c`** — Update `handle_file_menu()` for New Window (Cmd-N) and
  Close Window (Cmd-W) menu items; update `update_menus()` to disable New Window
  when at max sessions

### Key Code Changes

**Window cascading** (20px offset per slot, per UI/UX review):
```c
#define CASCADE_OFFSET  20

/* In session_new(): */
offset = s->id * CASCADE_OFFSET;
SetRect(&bounds, 2 + offset, 42 + offset,
    SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2);
s->window = NewWindow(0L, &bounds, "\pGeomys", true,
    documentProc, (WindowPtr)-1L, true, 0L);
```

**New Window flow:**
1. Find empty slot in `sessions[]`
2. `NewPtr(sizeof(BrowserSession))`, `memset` to zero
3. Create window with cascading bounds
4. Create scrollbar, TextEdit, init gopher engine
5. Navigate to home page (or show blank if none configured)
6. Make new window active via `SelectWindow()`

**Close Window flow:**
1. If page is loading, show CautionAlert: "Stop loading and close?"
2. Close TCP connection if active
3. Dispose scrollbar, TEHandle, window
4. Clear slot, decrement count
5. If was active session: set `active_session = session_from_window(FrontWindow())`
6. If no sessions remain: set `g_running = false` (quit)

**File menu update:**
```
File
  New Window       Cmd-N    (NEW)
  ─────────────
  Open URL...      Cmd-L    (unchanged)
  Save Page As...  Cmd-S    (unchanged)
  ─────────────
  Close Window     Cmd-W    (renamed from "Close")
  ─────────────
  Quit             Cmd-Q    (unchanged)
```

### Resource Changes

- **`resources/geomys.r`** — Update File menu MENU resource: add "New Window"
  item with Cmd-N shortcut, rename "Close" to "Close Window", update item
  numbering constants

### Testing Checkpoint

- Cmd-N opens a new window at cascaded position
- New window navigates to home page
- Click close box or Cmd-W closes the window
- Closing last window quits the application
- Close box on loading window shows confirmation alert
- Cmd-N disabled (grayed) when at maximum window count
- Windows appear in correct cascade positions

### Risk Areas

- **File menu item renumbering**: Adding "New Window" shifts all item indices.
  Must update all `FILE_MENU_*` constants in `main.h` and all references in
  `menus.c` and `main.c`.
- **TEHandle creation for new windows**: Must call `SetPort(win)` before
  `TENew()` so the TE is created in the correct port.
- **Memory exhaustion**: `NewPtr` for BrowserSession or `NewWindow` can fail
  on low memory. `session_new()` must check every allocation and clean up
  partial state on failure.

---

## Phase 3: Event Dispatch — Multi-Window Event Routing

**Goal:** Route all events correctly to the appropriate session. Background
connections continue loading while the user interacts with another window.

### Files to Modify

- **`src/main.c`** — Rewrite event loop handlers to route by window:
  - `nullEvent`: Poll ALL sessions with active connections (background loading);
    cursor blink/hover only for active session
  - `mouseDown/inContent`: Look up session via `session_from_window(win)`;
    save/restore state on session switch
  - `mouseDown/inDrag`: Allow dragging any session's window
  - `mouseDown/inGrow`: Resize correct session's window
  - `mouseDown/inGoAway`: Close correct session
  - `updateEvt`: Draw correct session's window (temporarily load its state)
  - `activateEvt`: Switch `active_session` on window activation/deactivation
  - `keyDown`: Always routes to `active_session`
- **`src/content.c`** — Ensure `content_draw()`, `content_resize()` work when
  called for a non-active session (port must be set to correct window)

### Key Code Changes

**Background connection polling:**
```c
/* In nullEvent handler: */
for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
    s = sessions[i];
    if (!s || !s->gopher.receiving) continue;

    /* Throttle background sessions: poll every 4th tick */
    if (s != active_session && (tick_count & 3) != 0) continue;

    /* Temporarily swap state for this session */
    session_save_state(active_session);
    session_load_state(s);
    SetPort(s->window);

    if (gopher_idle(&s->gopher)) {
        content_draw(s->window);
        content_update_scroll(s->window);
    }
    if (!s->gopher.receiving) {
        /* Connection complete: update status, cache, title */
        /* ... same logic as current single-window ... */
    }

    session_save_state(s);  /* save back */
}
/* Restore active session */
session_load_state(active_session);
if (active_session) SetPort(active_session->window);
```

**WaitNextEvent timing update:**
```c
wait_ticks = 10L;
if (g_suspended) {
    wait_ticks = 60L;
} else {
    for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
        if (sessions[i] && sessions[i]->gopher.receiving) {
            wait_ticks = 0L;
            break;
        }
    }
}
```

**Activate/deactivate handler:**
```c
case activateEvt:
    win = (WindowPtr)event->message;
    s = session_from_window(win);
    if (s) {
        if (event->modifiers & activeFlag) {
            if (active_session && active_session != s)
                session_save_state(active_session);
            active_session = s;
            session_load_state(s);
            browser_activate(true);
            content_activate(win, true);
        } else {
            browser_activate(false);
            content_activate(win, false);
            session_save_state(s);
        }
    }
    break;
```

### Testing Checkpoint

- Open 2+ windows, navigate to different pages in each
- Click between windows: correct page, URL, scroll position, history shown
- Start loading in window A, click to window B: B is responsive, A continues
  loading in background
- Window A loading completes while B is active: A's title/status update correctly
- Resize window B while A is loading: both windows draw correctly
- Back/forward history is independent per window
- Drag any window by title bar: correct window moves
- Click content in inactive window: first click activates, second click navigates

### Risk Areas

- **State corruption from incomplete save/restore**: If `session_save_state()`
  is called without a matching `session_load_state()` (or vice versa), module
  statics become inconsistent. Must audit every early-return path in event
  handlers. Use a helper:
  ```c
  void session_switch_to(BrowserSession *s) {
      if (s == active_session) return;
      if (active_session) session_save_state(active_session);
      active_session = s;
      session_load_state(s);
      SetPort(s->window);
  }
  ```
- **Offscreen buffer with non-active windows**: `offscreen_begin/end` assumes
  the current port. When drawing a background session's window during updateEvt,
  the offscreen buffer works because `SetPort(s->window)` is called first and
  `offscreen_begin()` operates on whatever the current port is. Should still
  be verified with testing.
- **Scroll bar action proc**: The `scroll_action()` callback gets the
  ControlHandle and derives the window from `(*ctl)->contrlOwner`. This works
  because each session's scrollbar is owned by its window. But the content
  module statics must be loaded for the correct session. Since scrollbar
  tracking happens synchronously within `TrackControl()`, this is safe as
  long as the correct session is active when the user clicks.

---

## Phase 4: UI Integration — Window Menu & Menu State

**Goal:** Add the Window menu for switching between windows, update all menus
to reflect active window state.

### Files to Create or Modify

- **`resources/geomys.r`** — Add MENU resource for Window menu (ID 133);
  update MBAR resource to include Window menu between Favorites and Options;
  renumber Options menu ID if needed
- **`src/main.h`** — Add `WINDOW_MENU_ID`, update `OPTIONS_MENU_ID` if
  renumbered, add Window menu item constants
- **`src/menus.c`** — Add `window_menu` handle; implement
  `update_window_menu()` to dynamically rebuild window list on each menu pull;
  implement `handle_window_menu()` for window switching; update `init_menus()`
  to load Window menu; update `update_menus()` to call `update_window_menu()`
- **`src/session.c`** — Add `session_get()` (get by index) for menu enumeration
- **`src/session.h`** — Declare `session_get()`

### Key Code Changes

**Window menu structure:**
```
Window
  N of M Windows        (disabled informational header)
  ─────────────
  checkmark  Geomys - sdf.org          (window 1 title)
             Geomys - example.com      (window 2 title)
             Geomys - host3            (window 3 title)
```

**Dynamic menu rebuild:**
```c
void update_window_menu(void) {
    short i, item_num;
    char label[80];
    Str255 ps;

    /* Clear all items after separator (item 2) */
    while (CountMItems(window_menu) > 2)
        DeleteMenuItem(window_menu, CountMItems(window_menu));

    /* Header: "N of M Windows" */
    snprintf(label, sizeof(label), "%d of %d Windows",
        session_count(), GEOMYS_MAX_WINDOWS);
    c2pstr(ps, label);
    SetMenuItemText(window_menu, 1, ps);
    DisableItem(window_menu, 1);

    /* List open windows */
    item_num = 3;
    for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
        BrowserSession *s = session_get(i);
        if (!s) continue;

        get_window_title(s, label, sizeof(label));
        c2pstr(ps, label);
        AppendMenu(window_menu, "\p ");
        SetMenuItemText(window_menu, item_num, ps);

        /* Checkmark on active window */
        CheckItem(window_menu, item_num,
            s == active_session);
        item_num++;
    }
}
```

**Menu state per active window** — `update_menus()` changes:
- File > Save Page As: enabled if `active_session->gopher.page_type != PAGE_NONE`
  and `active_session->app_state == APP_STATE_IDLE`
- File > New Window: disabled if `session_count() >= GEOMYS_MAX_WINDOWS`
- Edit menu: based on `active_session`'s focus state and selection
- Favorites > Add: uses `active_session`'s current URL and title

### Conditional Compilation

When `GEOMYS_MAX_WINDOWS == 1`:
- Window menu is **not** added to menu bar (no MBAR change needed)
- `update_window_menu()` compiles to no-op
- File menu does **not** include "New Window" item
- Close Window behaves as Quit (current behavior)

### Testing Checkpoint

- Window menu appears between Favorites and Options
- Window menu shows correct count header ("2 of 4 Windows")
- Clicking window menu item switches to that window
- Checkmark appears next to active window
- Window titles update when pages finish loading
- New Window grayed when at max count
- Edit menu Copy enabled/disabled matches active window's selection
- File > Save Page As reflects active window's page state
- Favorites > Add Favorite adds active window's URL

### Risk Areas

- **Menu ID conflicts**: Adding Window menu between Favorites (131) and
  Options (132) requires either renumbering Options to 133+ or inserting
  Window at a non-sequential ID. Recommend: use Window menu ID 133, bump
  Options to 134, Font to 135, Style to 136. Must update ALL references
  in `main.h`, `menus.c`, and `geomys.r`.
- **AppendMenu metacharacter interpretation**: `AppendMenu` interprets
  special characters in the string (`;`, `^`, `!`, `<`, `/`, `(`).
  Window titles from Gopher servers may contain these. Must use
  `AppendMenu("\p ")` then `SetMenuItemText()` to set the actual text
  safely, avoiding metacharacter issues.
- **Menu rebuild cost**: Rebuilding the Window menu on every `MenuSelect()`
  is fine — it's at most 4 items. Flynn does this and it's imperceptible.

---

## Phase 5: Build Presets & Memory Scaling

**Goal:** Wire `GEOMYS_MAX_WINDOWS` into the build system with appropriate
presets and memory calculations.

### Files to Modify

- **`CMakeLists.txt`** — Add `GEOMYS_MAX_WINDOWS` variable (default 1),
  add `CACHE_MAX` override based on window count, add `src/session.c`
  conditionally, pass definitions to compiler
- **`scripts/build.sh`** — Add `GEOMYS_MAX_WINDOWS` to preset definitions,
  add `--max-windows N` CLI flag, update `compute_size()` to account for
  per-session memory, update build summary output
- **`src/cache.c`** / **`src/cache.h`** — Make `CACHE_MAX` overridable via
  compile definition (use `#ifndef CACHE_MAX` guard); update cache keying
  to be per-session when `GEOMYS_MAX_WINDOWS > 1`

### Key Code Changes

**Preset definitions:**

| Preset | `GEOMYS_MAX_WINDOWS` | Cache Slots | SIZE Preferred | SIZE Minimum |
|--------|---------------------|-------------|---------------|-------------|
| Minimal | 1 | 3 | 416 KB | 288 KB |
| Lite | 2 | 4 | 576 KB | 448 KB |
| Full | 4 | 6 | 896 KB | 768 KB |

**build.sh changes:**
```bash
GEOMYS_MAX_WINDOWS=1  # default

apply_preset() {
    case "$1" in
        minimal)   GEOMYS_MAX_WINDOWS=1  ;;
        lite)      GEOMYS_MAX_WINDOWS=2  ;;
        full)      GEOMYS_MAX_WINDOWS=4  ;;
    esac
}

# CLI flag
--max-windows)  GEOMYS_MAX_WINDOWS="$2"; shift 2 ;;

# Updated SIZE calculation
compute_size() {
    local per_session=93
    local sessions=$GEOMYS_MAX_WINDOWS
    # ... shared costs (same as current) ...
    local session_total=$(( per_session * sessions ))
    local computed=$(( base + shared + session_total + cache_kb ))
    SIZE_PREFERRED=$(( computed * 130 / 100 ))
    SIZE_MINIMUM=$(( SIZE_PREFERRED - 128 ))
}
```

**CMakeLists.txt changes:**
```cmake
if(NOT DEFINED GEOMYS_MAX_WINDOWS)
    set(GEOMYS_MAX_WINDOWS 1)
endif()

if(NOT DEFINED GEOMYS_CACHE_SLOTS)
    if(GEOMYS_MAX_WINDOWS GREATER 2)
        set(GEOMYS_CACHE_SLOTS 6)
    elseif(GEOMYS_MAX_WINDOWS GREATER 1)
        set(GEOMYS_CACHE_SLOTS 4)
    else()
        set(GEOMYS_CACHE_SLOTS 3)
    endif()
endif()

target_compile_definitions(Geomys PRIVATE
    GEOMYS_MAX_WINDOWS=${GEOMYS_MAX_WINDOWS}
    $<$<BOOL:${GEOMYS_CACHE}>:CACHE_MAX=${GEOMYS_CACHE_SLOTS}>
)

if(GEOMYS_MAX_WINDOWS GREATER 1)
    list(APPEND GEOMYS_SOURCES src/session.c)
endif()
```

**Cache per-session keying** (when `GEOMYS_MAX_WINDOWS > 1`):
```c
/* cache key: combine session_id and history_idx */
typedef struct {
    short session_id;
    short history_idx;
    /* ... cached page data ... */
} CacheEntry;
```

### Testing Checkpoint

- `./scripts/build.sh --preset minimal` builds with `GEOMYS_MAX_WINDOWS=1`
- `./scripts/build.sh --preset lite` builds with `GEOMYS_MAX_WINDOWS=2`
- `./scripts/build.sh --preset full` builds with `GEOMYS_MAX_WINDOWS=4`
- `./scripts/build.sh --max-windows 3` overrides preset
- SIZE resource values in .r file match calculated values
- Build summary prints window count
- Minimal preset compiles with zero multi-window overhead (no session.c)
- Cache works correctly per-window (back in window A doesn't return window B's page)

### Risk Areas

- **Cache key collision**: Must ensure cache entries are scoped per-session.
  The simplest approach: multiply `CACHE_MAX` by window count and use
  `(session_id * HISTORY_MAX + history_idx)` as the slot key. More complex
  but fairer: LRU eviction across all sessions.
- **SIZE resource stamping**: `build.sh` uses `sed` to replace SIZE values
  in `geomys.r`. Must update the sed patterns to handle the new computed
  values. Test with all three presets.

---

## Phase 6: Polish — Performance, Edge Cases, Inactive Window Treatment

**Goal:** Optimize performance, handle edge cases, implement proper inactive
window visual treatment per Apple HIG.

### Files to Modify

- **`src/main.c`** — Optimize save/restore to skip history copy on background
  polling (only copy gopher state); first-click-activates behavior for inactive
  windows
- **`src/browser.c`** — Inactive window treatment: draw nav buttons as dimmed,
  address bar as static text (no focus ring/caret), when `win_active == 0`
- **`src/content.c`** — Inactive window: hide scrollbar thumb/arrows via
  `HiliteControl(scrollbar, 255)` on deactivate, restore on activate; disable
  hover effects for inactive windows; hide selection highlight per HIG
- **`src/session.c`** — Add `session_any_loading()` for WaitNextEvent timing;
  optimize `session_from_window()` with active_session fast check
- **`src/menus.c`** — Ensure all menu state reflects active window on every
  activation change, not just on menu bar click

### Key Code Changes

**Optimized save/restore for background polling:**
```c
/* Lightweight save/restore: only gopher + content state,
 * skip history (6.6 KB) — background polling doesn't touch history */
void session_save_gopher(BrowserSession *s);
void session_load_gopher(BrowserSession *s);
```

**First-click-activates** (per HIG p.140):
```c
case inContent:
    if (win != FrontWindow()) {
        /* First click only activates — does NOT perform action */
        SelectWindow(win);
    } else {
        /* Window already active — handle click normally */
        /* ... */
    }
    break;
```

**Inactive window scrollbar treatment:**
```c
/* On deactivation: */
if (s->scrollbar)
    HiliteControl(s->scrollbar, 255);  /* dim: no thumb, no arrows */

/* On activation: */
content_update_scroll(s->window);  /* restores correct hilite state */
```

**Optimized session_from_window:**
```c
BrowserSession *session_from_window(WindowPtr win) {
    short i;
    if (!win) return NULL;
    /* Fast path: check active session first (most common case) */
    if (active_session && active_session->window == win)
        return active_session;
    for (i = 0; i < GEOMYS_MAX_WINDOWS; i++) {
        if (sessions[i] && sessions[i]->window == win)
            return sessions[i];
    }
    return NULL;
}
```

### Additional Polish Items

- **Window title during loading**: Show "Loading host..." while connection
  is in progress, update to "Geomys - Page Title" when complete
- **Zoom box behavior**: Zoom toggles between user size/position and
  near-full-screen. Each window tracks its own user state rect.
- **DA window handling**: `SystemClick()` for DA windows must not interfere
  with session lookup. `session_from_window()` returns NULL for DA windows
  (negative `windowKind`), which is correct.
- **Memory pressure**: If `session_new()` fails due to low memory, show
  a StopAlert explaining insufficient memory. Do not crash.

### Testing Checkpoint

- Open 4 windows, click between them rapidly: no visual glitches
- Background loading: window A loads while browsing window B, title updates
  correctly when complete
- Inactive window: no caret blink, dimmed scrollbar, no hover highlight,
  no link underline
- First click on inactive window: only activates, does not navigate
- Scroll position preserved when switching between windows
- Text selection preserved when switching away and back
- Resize one window, switch to another: both draw correctly
- Close all windows: application quits cleanly
- Memory usage stable after opening/closing many windows (no leaks)
- Zoom box works per-window
- DA windows (Calculator, etc.) work correctly alongside browser windows

### Risk Areas

- **TEIdle for inactive windows**: Currently `browser_idle()` calls
  `TEIdle(g_addr_te)` for cursor blink. Must only call for active session's
  TEHandle. If called for an inactive session, the caret blinks in a hidden
  window. Solution: `browser_idle()` already only runs for `active_session`.
- **Offscreen buffer reuse**: When switching between windows, the offscreen
  buffer is used for whichever window calls `content_draw()`. This is safe
  because `offscreen_begin()` redirects to the buffer and `offscreen_end()`
  blits to the current port. No stale data persists.
- **68000 performance**: Save/restore memcpy of ~350 bytes (excluding history)
  takes <0.1 ms on 8 MHz 68000. History copy (6.6 KB) takes ~1.7 ms. Both
  are imperceptible. The optimized gopher-only save/restore avoids the
  history copy during background polling.

---

## Documentation Updates

### CHANGELOG.md

Add entry for v0.3.0:
```
## [0.3.0] - YYYY-MM-DD
### Added
- Multi-window browsing: open up to 4 simultaneous Gopher windows
- Window menu for switching between open windows
- New Window (Cmd-N) and Close Window (Cmd-W) commands
- Background loading: pages load in background windows while browsing
- Per-window history, scroll position, and text selection
- GEOMYS_MAX_WINDOWS build flag (1/2/4) with preset integration
- Build presets scale memory and cache allocation per window count

### Changed
- File menu reorganized: New Window, Open URL, Save Page As, Close Window, Quit
- Close Window (Cmd-W) replaces Close; closing last window quits application
- Cache pool shared across windows with scaled slot allocation
```

### README.md

- Update feature list to mention multi-window support
- Update build instructions to document `--max-windows` flag
- Update preset table with window counts
- Add screenshot showing multiple windows cascaded

### About Geomys (docs/About Geomys)

- Update version to 0.3.0
- Add "Multi-window browsing" to feature list

---

## Implementation Order Summary

| Phase | Description | Est. Scope | Dependencies |
|-------|------------|-----------|-------------|
| 1 | Foundation: BrowserSession struct, save/restore, single-window refactor | Large | None |
| 2 | Window lifecycle: New Window, Close Window, cascading | Medium | Phase 1 |
| 3 | Event dispatch: multi-window routing, background loading | Large | Phase 2 |
| 4 | UI integration: Window menu, menu state, keyboard shortcuts | Medium | Phase 3 |
| 5 | Build presets: GEOMYS_MAX_WINDOWS, memory scaling, cache | Medium | Phase 4 |
| 6 | Polish: inactive treatment, performance, edge cases | Medium | Phase 5 |

Each phase produces a buildable, testable binary. Phases should be merged to
main via squash commits with clear descriptions.

---

## Memory Budget Summary

| Preset | Windows | Per-Session | Shared | Cache | Total (typ.) | +30% Headroom |
|--------|---------|-------------|--------|-------|-------------|--------------|
| Minimal | 1 | 93 KB | 137 KB | 90 KB | 320 KB | 416 KB |
| Lite | 2 | 186 KB | 137 KB | 120 KB | 443 KB | 576 KB |
| Full | 4 | 372 KB | 137 KB | 180 KB | 689 KB | 896 KB |

All presets fit comfortably within the 4 MiB Mac Plus memory budget.
