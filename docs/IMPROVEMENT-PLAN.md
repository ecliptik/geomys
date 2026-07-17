# Geomys Improvement Plan

Generated 2026-07-17 from a multi-agent bug & performance review of `main`
(c9c168c). Seven reviewers swept the codebase across networking, rendering,
app-shell, parsing, state/persistence, memory safety, and 68000 performance;
every finding was then adversarially re-verified against the source. Four known
bugs (32 KB truncation, progressive rendering, dialog redraw gap, unreachable
scroll bar) were separately root-caused.

Severity uses the QA-PLAN rubric: **P0** = crash / heap corruption / data loss;
**P1** = broken feature or wrong behavior; **P2** = cosmetic / minor. Findings
are ordered by severity, then by blast radius. Line numbers are from `main` at
review time — re-confirm before editing.

Duplicate findings from the overlapping "memory" and "performance" cross-cut
passes have been merged; each item below is one distinct defect.

---

## Known-bug status (from root-cause pass)

| Known bug | Verdict | Action |
|-----------|---------|--------|
| 32 KB text truncation | **Still present** (deliberate cap) | Fix — see P1-4 |
| Progressive rendering regression | **Already fixed** in a77b1ac (v0.15.5), QA-verified | Optional hardening — see H-1 |
| External URL / telnet dialog redraw gap | **Not present** on `main` (stale TODO, was pre-fix `feature/internet-config`) | Retest in Snow, close TODO; but see P1-13 for a real latent bug found in the same code |
| Scroll bar unreachable at x=495–509 (minimal) | **Not present** — routing is FindControl-first; symptom is Snow XTEST coordinate scaling (~1.63×) landing 1–2 px left of the bar | Aim QA clicks at column center (~x502); optional geometric guard — see H-2 |

---

## P0 — Crashes & heap corruption (fix first)

### P0-1. HTML renderer overflows its text buffers (heap corruption on normal pages)
`src/html.c:68, 87` (text_buf) and `:106, 108` (text_lines).
For a `PAGE_HTML` page, `gopher_navigate` allocates `text_buf` at
`GOPHER_TEXT_INIT_SIZE` (8 KB) and `text_lines` at `GOPHER_INIT_TEXT_LINES`
(512). `html_emit`/`html_emit_run` bound writes against `GOPHER_TEXT_BUFSIZ`
(32 KB) and `html_emit_newline` bounds against `GOPHER_MAX_TEXT_LINES` (3000) —
the compile-time *maxima*, not the actual capacities — and **nothing in the HTML
path ever grows the buffers** (`gopher_grow_text_buf`/`gopher_grow_text_lines`
are called only from the plain-text and CSO paths).
- **Failure:** any `h`-type page whose stripped text exceeds 8 KB (trivial for a
  normal page, fully attacker-controlled) writes up to ~24 KB past the 8 KB
  block; >511 line-producing tags overrun the 2 KB line array by ~10 KB.
- **Fix:** mirror the plain-text path — bound-check against
  `gs->text_buf_capacity` / `gs->text_lines_capacity` and call the grow helpers
  (export them, or route the check through `gopher.c` before dispatch).
- **Reachability:** default-on in full & lite presets (`GEOMYS_HTML`).

### P0-2. `cache_retrieve` overflows the line index on Back navigation
`src/cache.c:428` (restore memcpy) and `:456` (rebuild loop).
The items and text_buf restore paths reallocate when capacity is too small, but
the `text_lines` path only allocates when `gs->text_lines == NULL`; otherwise it
`memcpy`s `slot->text_line_count` longs into whatever buffer exists. The
rebuild-fallback loop is bounded by `GOPHER_MAX_TEXT_LINES` (3000), not the
512-entry allocation.
- **Failure:** view a large cached text page (line index grown toward 3000),
  navigate to a small text page (index reallocated at 512), press **Back** →
  `memcpy` of up to 12 KB into a 2 KB block → app-heap corruption on a routine
  Back.
- **Fix:** if `gs->text_lines` is non-NULL but
  `text_lines_capacity < slot->text_line_count`, dispose & reallocate (mirror
  the text_buf branch); bound the rebuild loop by `text_lines_capacity`. Also
  fix `cache.c:420-422/443-445`, which set `text_lines_capacity` *before*
  checking `NewPtr` for NULL (stale capacity paired with nil pointer).
- **Reachability:** `GEOMYS_CACHE` (default-on in full preset).

### P0-3. Gopher+ `+--` error flips a directory page to text with no buffer
`src/gopher.c:565`.
On a Gopher+ `+--` error status, `gopher_process_data` unconditionally sets
`page_type = PAGE_TEXT`. But if the request was a directory / search / alternate
view / ASK form (`gplus_view`/`gplus_ask_form`, reachable from the Get Info
dialog), only `items` was allocated — `text_buf` is NULL and capacity 0. The
text path then computes `new_cap = 0*2 = 0`, `NewPtr(0)` returns a zero-length
block, `avail = cap - 1 - text_len = -1` → `copy_len = -1` → `memchr(p,'\r',
(size_t)-1)` scans all of memory (bus error on 68030), and `text_buf[0]='\0'`
smashes the adjacent block header.
- **Failure:** any Gopher+ server returns `+--` to an ASK submission or alternate
  view on a type-1/type-7 item → heap corruption / crash.
- **Fix:** in the `+--` branch, only switch to `PAGE_TEXT` after ensuring a text
  buffer exists (allocate one, or guard the text path against `capacity == 0`).
- **Reachability:** `GEOMYS_GOPHER_PLUS` (default-on in full & lite).

### P0-4. `conn_close` reuses a still-pending async TCP parameter block
`src/connection.c:394` → `src/tcp.c:236` (`_TCPAbort`) and `:277`
(`_TCPRelease`).
`conn_connect` issues `TCPActiveOpen` asynchronously on `conn->pb`. While the
open is in flight (`ioResult == 1`, pb owned by the Device Manager and queued),
`conn_close` calls `_TCPAbort(&conn->pb,…)` then `_TCPRelease(&conn->pb,…)`, and
both begin with `memset(pb, 0, sizeof(*pb))` — zeroing `qLink`/`ioResult` of a
queued pb and re-issuing `PBControl` on it. Classic "never reuse a pending pb"
violation → driver I/O queue corruption, hang, or crash.
- **Failure (reliable triggers):** user navigates / Back / Forward / closes the
  window / quits **while a connect is pending** — `gopher_navigate` and
  `gopher_cleanup` call `conn_close` on any non-idle state, and the event loop
  keeps handling input during load. (The 30 s app-timeout branch is a rarer
  trigger; MacTCP's own 10 s ULP timeout usually completes the pb first.)
- **Fix:** don't reuse `conn->pb` until `ioResult != 1`. Abort a pending open
  with a *separate* pb (or poll for completion before releasing). Cover both
  `_TCPAbort` and `_TCPRelease`.

### P0-5. Closing the only window leaves dangling statics → next keypress is use-after-free
`src/main.c:1522` (+ `src/browser.c` statics, `src/menus.c:375`).
In single-window builds (`GEOMYS_MAX_WINDOWS == 1`) `active_session` is
`&g_single_session` — never NULL — so every `if (!active_session)` guard is dead
code. The user can still close the only window (the app keeps running by
design); `session_destroy` disposes `s->addr_te` and nulls `s->window`, but
`browser_cleanup` is **never called anywhere**, so `g_addr_te` still points at
the freed TEHandle, and history/refresh statics stay stale.
- **Failure:** minimal preset → load a page → close the window → press any letter
  → dead `!active_session` check passes → `SetPort(NULL)` then `TEKey` on the
  freed `g_addr_te` → heap corruption. Go > Back/Refresh stay enabled and crash
  the same way.
- **Fix:** on single-window `session_destroy`, call `browser_cleanup` (and reset
  content/history module statics), and disable the stale Go/Refresh items.

### P0-6. `autoKey` handler lacks the NULL-session guard that `keyDown` has
`src/main.c:1226` (multi-window presets only).
The `keyDown` case guards `if (!active_session) {…cmd only…; return;}`; the
`autoKey` case does not, and its else-branch does `SetPort(g_window)` +
`browser_key` unconditionally. In lite/full builds, after the last window
closes `active_session` is genuinely NULL.
- **Failure:** close all windows, hold any character key → first repeat is an
  `autoKey` → `g_window` (= `active_session->window`) dereferences NULL → garbage
  port → `TEKey` on stale `g_addr_te` → crash / corruption.
- **Fix:** add the same NULL guard to the `autoKey` case.

### P0-7. `update_menus` no-session branch leaves Favorites/Options enabled → NULL deref / UAF
`src/menus.c:375` (multi-window presets).
When the last window closes and `active_session == NULL`, the no-session branch
disables File/Edit/Go/Window but never disables `favorites_menu` or
`options_menu`.
- **Failure:** load a page, close all windows, choose Favorites > Add → executes
  `gopher_build_uri(…, g_gopher.cur_host, …)` / `GetWTitle(g_window,…)` through a
  NULL `active_session` → crash. Options > Home Page dereferences the freed
  `g_addr_te` (same root cause as P0-5: `browser_cleanup` never called).
- **Fix:** disable `favorites_menu`/`options_menu` (and `directory_menu`) in the
  no-session branch; fix the `browser_cleanup` leak.

---

## P1 — Wrong behavior & broken features

### P1-1. History-full shift desynchronizes the page cache → Back shows the wrong page
`src/history.c:32-37` + `src/main.c:2666` (`cache_retrieve` keyed by history
index). `HISTORY_MAX` is 10; when full, `history_push` shifts entries left one
slot, but the cache is keyed by `(session_id, history_index)` and is never
re-keyed or invalidated for the shift.
- **Failure:** after the 11th navigation, `Back` to index N returns the content
  cached for the *pre-shift* entry N while the address bar/title show entry N's
  new URL — silent content/URL mismatch; clicking a link then goes somewhere
  unintended. (Bounded by `CACHE_MAX` = 3/4/6, so only the few most-recent
  indices are stale.)
- **Fix:** store host/port/selector in the `CacheSlot` and validate on retrieve
  (or re-key/invalidate the cache on shift). `GEOMYS_CACHE` = full preset.

### P1-2. Fully synchronous DNS freezes the UI for 10–40 s
`src/dns.c:373` (also the top performance finding).
`dns_resolve` runs on the main thread with no `WaitNextEvent`: up to 2×5 s
blocking UDP receives, then a synchronous TCP fallback (`_TCPActiveOpen`
10–30 s). The actual Gopher connection is properly async-polled — DNS is the one
remaining full-UI freeze.
- **Failure:** navigate to an uncached hostname with a slow/unreachable resolver
  (or a NAT that drops UDP — the project's own Snow setup) → app frozen, watch
  cursor, no cancel, MultiFinder starved for 10+ s. Directly contradicts the
  stated latency priority.
- **Fix:** convert DNS to the same async-poll pattern used for the Gopher
  connection (async UDP/TCP with `ioResult` polling from `conn_idle`).

### P1-3. High ports (32768–65535) collapse to port 70
`src/gopher.c:1193`. `*port = (short)atoi(p); if (*port <= 0) *port =
GOPHER_DEFAULT_PORT;` — any port ≥ 32768 casts to a negative short and is
silently rewritten to 70. `gopher_build_uri` then renders
`gopher://host:-32503/…`, which is what gets bookmarked and re-parsed.
- **Failure:** typing/opening `gopher://host:33033/` connects to port 70; menu
  navigation round-trips through `build_uri`→`parse_uri` and breaks too.
- **Fix:** use an unsigned 16-bit port throughout (`unsigned short`), parse with
  range check `1..65535`, format with `%u`.

### P1-4. Text pages truncated at 32,767 bytes (known bug — still present)
`src/gopher.h:16` (`GOPHER_TEXT_BUFSIZ = 32*1024`).
Deliberate cap, not an overflow: `gopher_process_data` stops growing at 32 KB
and clamps each copy to the remaining space, silently dropping the rest
(`avail = cap - 1 - text_len` saturates at 32767). Counters are already 32-bit
`long`; rendering uses custom `DrawText`, not TextEdit — so the cap is the only
limit.
- **Fix:** make `GOPHER_TEXT_BUFSIZ` preset-aware (e.g. 64 KB minimal, 128–256 KB
  lite/full) using the existing doubling growth (which already handles `NewPtr`
  failure); raise `GOPHER_MAX_TEXT_LINES` proportionally (keep < 32767 —
  `text_line_count` is a short); surface a "(truncated)" status when the cap is
  still hit instead of dropping silently. Also clamp the Show Clipboard
  `TESetText` length to ~32000 (real TextEdit limit).
- **Note:** P0-1 and P0-2 must land first — they are the *unsafe* siblings of
  this cap.

### P1-5. Use-after-free of item pointers during navigation
`src/gopher.c:318` and callers at `src/main.c:2059/2196` (+ `history_push`).
`navigate_gopher_item` passes raw pointers into the current page's `items` array
(`item->host`, etc.) into `gopher_navigate`, which frees that array via
`gopher_clear_page` *before* `strncpy(gs->cur_host, host, …)` reads it; callers
then `history_push` the same dangling pointers.
- **Failure:** works today only because no allocation intervenes; any future
  allocation or heap compaction in that window turns the hostname into garbage
  that propagates into `cur_host`, the address bar, and history.
- **Fix:** copy host/selector/display into locals before `gopher_clear_page`
  runs. (Selector from the search/CSO dialogs is already a safe local copy.)

### P1-6. Back to a cached page mid-load leaves `receiving` stuck true → busy-poll forever
`src/main.c:2673`. The cache-hit path cancels the in-progress load
(`conn_close` → state IDLE, `g_app_state = IDLE`) but never clears
`g_gopher.receiving`; with state IDLE, `gopher_idle` matches no branch and never
clears it.
- **Failure:** click a link, press Back to a cached page while it loads → the
  event loop sees `receiving == true`, pins `wait_ticks = 1`, busy-polls at
  ~60 Hz indefinitely, starving MultiFinder / draining battery. Stop / Cmd-.
  can't clear it (`do_cancel_loading` needs `APP_STATE_LOADING`). Full preset.
- **Fix:** clear `receiving` (and drain state) in the cache-hit cancel path.

### P1-7. `prefs_save` deletes the old file before writing → total prefs loss on failure
`src/settings.c:164`. Unconditionally `PBHDeleteSync`s "Geomys Preferences"
first, then create/open/write; any failure (disk full, locked volume, crash) or
discarded `FSWrite` result leaves no/zero-length prefs — home URL, DNS server,
all favorites, theme, fonts gone, no error surfaced.
- **Fix:** write to a temp file, then delete + rename; check `FSWrite`/`FSClose`
  results.

### P1-8. Download cleanup drops the FSSpec `parID` → deletes the wrong directory
`src/savefile.c:474` (+ cleanup at `main.c:876/933/1453`). On System 7,
`start_save_to_disk` keeps only `vRefNum` + name, discarding `parID`; cleanup
calls `FSDelete(name, vRefNum)`, which resolves against the volume's default
directory, not the chosen folder.
- **Failure:** save into a subfolder and the connection fails mid-write → the
  partial file is left in the chosen folder, and a same-named file in the
  volume's default dir may be deleted instead.
- **Fix:** store the full FSSpec (or add `dl_parid`) and use `FSpDelete`/`HDelete`
  on the System 7 path.

### P1-9. Failed multi-step history jump corrupts the history position
`src/main.c:2772`. `navigate_history_to` (Go/Window history list) walks `g_pos`
multiple steps then navigates; on failure `navigate_history_entry` undoes only
*one* step.
- **Failure:** at index 5, pick index 1 from the menu, connection fails →
  `g_pos` left at 2 (never displayed) while the window shows page 5; Back/Forward,
  the Go checkmark, and cache indexing all now reference the wrong entry, and the
  next navigation truncates valid forward history.
- **Fix:** record the origin index in `navigate_history_to` and restore it fully
  on failure (the undo can't live in `navigate_history_entry`).

### P1-10. Home/End keys move the thumb but never redraw the content
`src/main.c:1657-1658`. The Home (0x01) / End (0x04) handlers call only
`content_set_scroll_pos` (updates `g_scroll_pos` + `SetControlValue`, no
dirty-mark, no draw). Every other caller follows it with
`content_mark_all_dirty` + `content_draw`.
- **Failure:** press Home/End on a scrolled page → thumb jumps but rows don't
  change; `g_scroll_pos` now disagrees with the shadow cache, so the next
  hover-row redraw mixes new-position and stale rows until a full redraw.
- **Fix:** follow `content_set_scroll_pos` with `content_mark_all_dirty` +
  `content_draw` (or call `content_vscroll_by`).

### P1-11. `--no-themes` build fails to link
`src/content.c:1372, 1502`. `content_draw_selection` is defined inside the
`#ifdef GEOMYS_THEMES` block but called under `#ifdef GEOMYS_CLIPBOARD`; nothing
forces THEMES on when CLIPBOARD is on, and `build.sh` surfaces `--no-themes`.
- **Failure:** `./scripts/build.sh --preset full --no-themes` → implicit
  declaration + undefined symbol at link.
- **Fix:** move the CLIPBOARD block out of the THEMES region (the function uses
  no theme helpers).

### P1-12. `content_draw` NULL-clip early return leaves the offscreen buffer active
`src/content.c:1568`. `offscreen_begin(win)` runs before the `if (!g_clip_rgn)
return;` guard, so an early return skips `offscreen_end`: the port stays
redirected to the offscreen bitmap/GWorld and `g_active` stays 1 forever.
- **Failure (reachable trigger):** per-window content re-init (multi-window /
  reopen) where region alloc fails under heap pressure → all subsequent drawing
  goes to the invisible buffer; app appears frozen.
- **Fix:** run the guard before `offscreen_begin` (or call `offscreen_end` on the
  early return).

### P1-13. `std_dlg_filter` returns true for background updates without setting `*item`
`src/dialogs.c:249-256` (surfaced by the dialog-redraw root cause).
The updateEvt branch returns true (dismissing `ModalDialog`) without setting
`*item`; callers declare `short item;` uninitialized (e.g. `do_html_url_dialog`,
`src/main.c:2383`).
- **Failure:** a background update arriving on the first `ModalDialog` iteration
  makes the do-while test read stack garbage; if it's 1 the dialog
  phantom-dismisses (and may launch the helper browser).
- **Fix:** set `*item = 0;` before `return true;` in the updateEvt branch (0 is
  not a valid item; every caller loop treats it as "keep going").

### P1-14. Shared `g_clip_rgn` clobbered re-entrantly during selection drag (System 7 color)
`src/content.c:2355`. On the color path, `track_content_drag` saves the window
clip into the single shared `g_clip_rgn`, then calls draw helpers that
`GetClip(g_clip_rgn)` themselves, overwriting the saved clip; the final
`SetClip(g_clip_rgn)` leaves the window clipped to the content rect.
- **Failure:** System 7 color — after any text-selection drag, direct
  (non-update) chrome draws (status hints, address-bar echo, button feedback)
  are clipped out until the next update event. Mono System 6 unaffected.
  Transient/cosmetic but user-visible on the full preset.
- **Fix:** use a distinct save region (or save/restore the clip with a local
  `RgnHandle`), don't share one region across nesting levels.

### P1-15. `drag.c` aliases a region handle → empty outline + double-dispose
`src/drag.c:216`. `*inner = *outer;` copies the master pointer, not the region
(should be `CopyRgn(outer, inner)`): `InsetRgn` mutates the shared block,
`DiffRgn` yields an empty outline, and both `DisposeRgn` calls free the same
block.
- **Fix:** `CopyRgn(outer, inner)`.
- **Reachability:** `GEOMYS_DRAG` is **off in all shipping presets** (only via
  `--drag`) — real heap corruption but non-default; treat as P1 gated on that
  flag.

---

## P2 — Correctness (low) & cosmetic

- **P2-1. RFC 1436 text terminator not handled** — `src/gopher.c:648`. Type-0
  responses keep a spurious trailing `.` line and don't un-stuff leading `..`.
  Strip the lone-`.` terminator and collapse `..`→`.` in the text path. (CSO
  path is fine — it terminates with `200:Ok.`.)
- **P2-2. Final directory line without trailing newline dropped** —
  `src/gopher.c:798`. Servers that FIN without a final `\n` (or omit `.\r\n`)
  lose the last item. Flush `line_buf` on the transition to `CONN_STATE_DONE`
  (strip trailing `\r`, skip a lone `.`).
- **P2-3. Loading status count invisible** — `src/main.c:665`. The hot-path
  formatter `memcpy`s 10 bytes of `"Loading\311 "` including the NUL into
  `prog[9]`, so `strlen` stops before the digits. Copy 9 bytes, `p += 9`.
- **P2-4. Selection/hit-test 2 px offset** — `src/content.c:2147`. Text drawn at
  `CONTENT_LEFT_MARGIN` (2) but hit-tested/highlighted at `TEXT_X_OFFSET` (4):
  highlight is 2 px right of glyphs; click→column mapping biased. Unify the
  constants.
- **P2-5. Astral-plane codepoints mis-mapped** — `src/utf8.c:202`. 4-byte code
  points are truncated to `unsigned short` before
  `unicode_symbol_to_macroman`, so U+12013 → en-dash, etc. Guard with
  `if (cp <= 0xFFFF)` before the cast.
- **P2-6. Out-of-scope stack pointer in `format_row_text`** — `src/content.c:906`.
  `disp` points at block-local `char xname[256]` dereferenced after the block
  ends (`memcpy` at :917) — UB if the optimizer reuses the slot. Hoist `xname`
  to function scope.
- **P2-7. Clipboard window unbounded `GetScrap` resize** — `src/menus.c:963`. The
  4096 clamp doesn't bound the handle; a large cross-app scrap forces a large
  resize (unchecked), and on failure shows uninitialized memory. Check the
  result; cap the resize.
- **P2-8. Stale `g_print_after_load` → spurious print** — `src/main.c:219`.
  `ae_print_doc` sets the flag before `do_navigate_url`; a malformed URL returns
  early without clearing it, so the next manually-loaded page is silently
  printed. Clear the flag on the early-return paths.
- **P2-9. Dirty-flag clear loop hard-codes 512** — `src/content.c:1812`.
  `g_dirty[]` spans `GOPHER_MAX_ITEMS` (2000) but the clear loop stops at 512, so
  rows ≥ 512 stay dirty and intermittently escalate single-row updates to full
  redraws. Bound by `GOPHER_MAX_ITEMS`.
- **P2-10. `content_init` leaks regions per window/reopen** — `src/content.c:311`.
  `g_clip_rgn`/`g_scroll_rgn` are re-`NewRgn`'d each `content_init` without
  disposing (module-shared, `content_cleanup` is dead code). Guard with
  `if (!g_clip_rgn)`. Small (~10–30 B each) but real on a 4 MB heap.
- **P2-11. `std_dlg_filter` blanks the download progress dialog** —
  `src/dialogs.c:253`. Update events for non-modal windows go to `handle_update`,
  which `EraseRect`s unknown windows; dragging a movable modal over the progress
  dialog erases it permanently (System 7). Handle `g_dl_dialog` in the filter or
  in `handle_update`.
- **P2-12. DNS-over-TCP breaks on segmented responses** — `src/dns.c:298`. Single
  `_TCPRcv` for length prefix and body (MacTCP returns on *any* data), plus a
  hardcoded 1 s receive timeout (declared 15 s never used) and a silent 512-byte
  clamp, and no retry. Add an accumulation loop gated on `amtUnreadData`. (Rare
  vs P1-2, but this is the *working* path in the Snow NAT env.)

---

## Performance backlog (68000 hot paths)

Latency is the project's stated top priority; these are ordered by likely
user-visible impact. P1-2 (sync DNS) is the biggest and is tracked above.

- **PERF-1. Full-page re-measure on load completion** — `src/content.c:3721`.
  `content_recalc_width` re-runs `format_row_text` + `TextWidth` over *every* row
  from 0 on load completion, ignoring `g_measured_rows` already accumulated
  incrementally. A 2000-item directory stalls seconds at "Done" redoing thousands
  of `TextWidth` traps. Skip rows below `g_measured_rows` (re-measure from
  `g_measured_rows - 1` for text pages, since the last line may have been
  partial).
- **PERF-2. O(n²) metadata ellipsis truncation** — `src/content.c:1304`. Show
  Details truncation shrinks one char at a time, calling `TextWidth` on the whole
  string each iteration (~5000 char-width computations/row), and runs even when
  the result is discarded (narrow window). Bail before the loop when
  `right_avail <= ellipsis_w`; binary-search the fit.
- **PERF-3. Arrow-key scroll does a full-page redraw** — `src/content.c:3058`.
  `content_vscroll_by` marks all dirty + full `content_draw` (~20–25 rows/keypress
  on a Plus) instead of the existing `ScrollRect` fast path used by the
  scrollbar arrows. Held-arrow scrolling saturates the CPU below the configured
  repeat rate. Route ±1 keyboard scroll through the `ScrollRect` path.
- **PERF-4. Unconditional `TextWidth` per drawn row** — `src/content.c:1149`.
  `content_draw_row` measures every row but uses it only for the hover underline
  (≤1 row) and the metadata path. Adds ~30–50% per-row text cost on full-page
  redraws. Compute lazily only when
  `row_index == g_hover_row || (split_pos > 0 && show_details)`.
- **PERF-5. `update_menus` redraws the whole menu bar every Cmd-key** —
  `src/menus.c:409` (+ 392). Every shortcut calls `update_menus` → unconditional
  `DrawMenuBar` + full Go-history teardown/rebuild (dozens of Menu Manager
  calls). Visible flash + latency on every Cmd-key. `DrawMenuBar` is only needed
  when bar items are added/removed; guard it and the history rebuild.
- **PERF-6. Status bar redrawn ~60×/s during connect** — `src/main.c:602`. While
  `CONN_STATE_OPENING`, `poll_active_session` re-`snprintf`s and re-draws
  identical "Connecting to …" text every tick (no changed-state check). Draw once
  on the transition into `OPENING`.
- **PERF-7. Selection-drag recomputes on a stationary mouse** — `src/content.c:2382`.
  `track_content_drag`'s `StillDown` loop calls `pixel_to_col` (full
  `format_row_text` + per-char `CharWidth` traps) every iteration before the
  row/col early-out; holding the button spins the CPU. Skip recomputation when
  the sampled `Point` hasn't moved.
- **PERF-8. `SICN` re-fetched per icon per row per repaint** — `src/gopher_icons.c:387`.
  `gopher_sicn_draw` calls `GetResource('SICN',…)` + HLock/HUnlock every call; the
  color path already caches (`cicn_get_cached`). Mostly mitigated by ScrollRect /
  shadow-cache row skipping, so this is a full-redraw-only cost — add a small
  handle cache mirroring the cicn path.

---

## Rejected during verification (do not action)

- `conn_send_selector` truncating Gopher+ requests > 256 B (`connection.c:432`) —
  refuted: bounded upstream.
- HTML run-length cast to short dropping long runs (`html.c:250`) — refuted:
  runs are bounded before the cast.

---

## Suggested sequencing

1. **P0-1, P0-2, P0-3** — the three heap-corruption bugs on normal/attacker
   content; land before the P1-4 cap change.
2. **P0-4 … P0-7** — the connection/close/NULL-session crash cluster (P0-5/6/7
   share the `browser_cleanup`-never-called root cause; fix together).
3. **P1-4** (cap) + **P1-1** (cache key) + **P1-3** (ports) + **P1-5** (UAF) —
   correctness of the core fetch/history/cache path.
4. **P1-2** (async DNS) — largest single latency win; sizeable, schedule its own
   branch.
5. Remaining P1 (state/persistence + build/link + rendering), then the
   performance backlog (PERF-1 … PERF-4 give the most perceptible speedup),
   then P2.

Each item lists a concrete fix; most are localized. Per QA-PLAN, verify one fix
at a time and don't chase new bugs found during verification — log them.

---

## Optional hardening (already-fixed known bugs)

- **H-1. Progressive rendering** (fixed in a77b1ac) — invalidate the shadow cache
  at *navigation* time (in `content_erase` / top of `do_navigate_url_titled`)
  rather than relying on during-load row-count detection, and make the shadow key
  page-aware, so stale slots from a previous page can never survive into a new
  load.
- **H-2. Scroll bar routing** (not a real bug) — optionally add a geometric guard
  so a dimmed-scrollbar click in the column is explicitly swallowed instead of
  falling through to `content_click`; and aim automated QA clicks at the column
  center (~x502) to avoid the Snow framebuffer scaling (~1.63×) landing 1–2 px
  left of the bar.
