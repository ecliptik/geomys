# Geomys Roadmap

## v0.1.0 — MVP

### Phase 1: Project Scaffolding & Build System
**Status: Complete**

Directory structure, build system, and minimal running app — empty window with menu bar and About dialog.

- Directory structure and CMakeLists.txt with feature flags
- `scripts/build.sh` with preset support
- Rez resources: menus, About dialog, SIZE, BNDL, FREF, ICN#
- Toolbox initialization and event loop
- Menu bar: Apple, File, Edit, Favorites, Options
- About Geomys dialog with version and build SHA
- Utility modules adapted from Flynn
- Initial documentation

### Phase 2: Networking & Gopher Protocol Core
**Status: Complete**

TCP connectivity and RFC 1436 Gopher protocol parser. Connect to a server, parse directory listings, display results.

- MacTCP wrapper and DNS resolver (from Flynn)
- Connection state machine: IDLE → RESOLVING → CONNECTING → SENDING → RECEIVING → DONE
- RFC 1436 line parser and GopherItem struct
- RFC 4266 URI parser (`gopher://host[:port]/type[selector]`)
- Type handler registry for all 18 item types
- Connection status window

### Phase 3: Browser Chrome & Window Layout
**Status: Complete**

Full browser UI — address bar, navigation buttons, status bar, menus wired.

- Nav bar: Back/Forward/Refresh/Home buttons + address bar (TextEdit)
- Content area with scroll bar
- Status bar: connection state and page info
- Open URL dialog (Cmd-L)
- Keyboard/mouse event routing
- Full menu enable/disable based on state

### Phase 4: Content Display — Directories & Text
**Status: Complete**

Scrollable content area with clickable directory listings and plain text files.

- Directory rendering: item type icons, clickable navigation, inverse highlight
- Text rendering: Monaco 9 monospaced, line offset index
- Scroll bar: page up/down, line up/down, thumb drag
- Efficient drawing: clipped to content area, visible items only

### Phase 5: Full Type Support & Search
**Status: Complete**

All 18 Gopher types handled. Type 7 (search) fully functional.

- Type 7 search query dialog
- Type 2 CSO phone-book query
- Type h HTML URL display
- Download types (4,5,6,9,d): informational messages
- Image types (g,I,p): informational messages
- Telnet types (8,T): suggest Flynn
- Error type (3): inline error display

### Phase 6: Navigation History & Error Handling
**Status: Complete**

Back/forward history, refresh, home page, address bar sync, robust error handling.

- 10-entry history stack (URL-only, re-fetch on navigate)
- Back/Forward/Refresh/Home button behavior
- Address bar and window title sync
- Cmd-[ / Cmd-] keyboard shortcuts
- Error handling: DNS failure, connection refused, timeout, partial content
- Cmd-. cancels in-progress connections

### Phase 7: Settings, Favorites & Release
**Status: Complete**

User preferences, favorites, release packaging.

- Settings: home page, DNS server, font choice (persisted)
- Favorites: add/edit/delete, dynamic menu, Cmd-D
- Build presets: minimal, lite, full
- Release scripts and documentation finalized

---

## v0.2.0 — Polish Pass

### Phase 8: Feature Flags & Cosmetic Fixes
**Status: Complete**

Modular build system with 10 feature flags, 3 presets, and cosmetic polish.

- 10 CMake feature flags with conditional compilation
- Build presets: minimal, lite, full (default)
- Command-line flags (`--flag`/`--no-flag` variants)
- Dynamic SIZE resource computed from enabled features
- Cosmetic fixes: grow box drawing, text clipping, navigation button icons

### Phase 9: Offscreen Double Buffering
**Status: Complete**

Flicker-free rendering via offscreen bitmap.

- Offscreen GrafPort for double-buffered drawing
- Feature-flagged with `GEOMYS_OFFSCREEN`
- CopyBits from offscreen buffer to window

### Phase 10: Font Selection & Hand Cursor
**Status: Complete**

User-selectable fonts and cursor feedback on navigable items.

- Font submenu: Monaco 9, Geneva 9, Chicago 12 (Options > Font)
- Hand cursor on navigable Gopher items (directories, links)
- Arrow cursor on non-navigable items

### Phase 11: CP437 & Unicode Glyphs
**Status: Complete**

Character set translation for proper display of BBS-style content.

- CP437 character set translation (box-drawing, special characters)
- Unicode glyph rendering engine (adapted from Flynn)
- Feature-flagged: `GEOMYS_CP437`, `GEOMYS_GLYPHS`
- CP437 auto-enables GLYPHS dependency

### Phase 12: Local Page Cache
**Status: Complete**

LRU cache for instant back/forward navigation.

- 3-page LRU cache
- Instant back/forward from cache (no re-fetch)
- Feature-flagged with `GEOMYS_CACHE`

### Phase 13: Search Query Preservation
**Status: Complete**

Search queries preserved in navigation history.

- Type 7 search queries stored with history entries
- Back/forward to search result pages without re-prompting

### Phase 14: Gopher+ Protocol Support
**Status: Complete**

Gopher+ awareness and metadata parsing.

- `has_plus` detection on directory items
- `+ADMIN` and `+VIEWS` attribute parsing
- Feature-flagged with `GEOMYS_GOPHER_PLUS`

### Phase 15: Page Styles & App Icon
**Status: Complete**

Visual presentation options and new application icon.

- Page format styles: Traditional, Plain, Markdown (Options > Page Style)
- Feature-flagged with `GEOMYS_STYLES`
- New pocket gopher application icon

---

## v0.2.1 — Performance & Polish

### Phase 1: Scroll + Hover Performance
**Status: Complete**

Extracted `content_draw_row()` from directory rendering loop. Line scroll uses `ScrollRect` + targeted row redraw. Hover redraws only 2 affected rows.

- `content_draw_row()` static helper: self-contained single-row drawing
- Line scroll: `ScrollRect` pixel shift + 1-2 row redraws (~15x faster)
- Hover: targeted 2-row redraws instead of full page (~12x faster)
- Page/thumb scroll unchanged (full redraw acceptable)

### Phase 2: Grow Box + Hand Cursor
**Status: Complete**

Grow box clip rect consistency and cursor update optimization.

- Grow box clip in `handle_update()` aligned with `content_draw()` (both use `SCROLLBAR_WIDTH` + 1)
- Cursor early-exit: skip redraws when hover row unchanged

### Phase 3: Page Styles
**Status: Complete**

Style branching in `content_draw_row()` for all 3 page styles.

- Traditional: type labels (`TXT`, `DIR`, etc.), monospaced alignment
- Plain: no type labels, underlines always shown on navigable items
- Markdown: bullet (`\xA5`) prefix for navigable items, `?` for search

### Phase 4: CP437/Glyph Integration
**Status: Complete**

CP437 translation in text page rendering with fast ASCII bypass.

- Fast path: scan for bytes > 0x7F, skip translation for ASCII-clean lines
- Slow path: character-by-character translation through `cp437_table[]`
- Glyph-type entries use `copy_char` fallback (no bitmap rendering on 68000)
- Feature-flagged with `GEOMYS_CP437`

### Phase 5: Gopher+ UI Indicator
**Status: Complete**

`+` suffix on type labels for Gopher+ items in Traditional style.

- Items with `has_plus` show `TXT+`, `DIR+`, etc.
- Feature-flagged with `GEOMYS_GOPHER_PLUS`

### Phase 6: Prefs Document Icon
**Status: Complete**

Fixed FREF local icon ID byte order for preferences document icon.

- FREF (129) corrected from `$"0100"` to `$"0001"` (matching Flynn)
- Desktop rebuild (Cmd-Option at startup) required for icon to appear

---

## v0.5.0 — 256-Color Themes

### Phase 1: Color Infrastructure
**Status: Complete**

Color QuickDraw detection and theme data structures.

- `color_detect()` via SysEnvirons — runtime Color QD check
- `GEOMYS_COLOR` and `GEOMYS_THEMES` feature flags
- `ThemeColors` struct with 15 color properties per theme

### Phase 2: Theme Engine & Built-in Themes
**Status: Complete**

9 built-in themes with monochrome/color awareness.

- Light, Dark (monochrome — all systems including Mac Plus)
- Solarized Light/Dark, Tokyo Night Light/Dark, Green Screen, Classic, Platinum (color — Mac II+)
- Theme headers compiled as const data, zero heap cost (~459 bytes)
- Color caching to minimize RGBForeColor/RGBBackColor trap calls

### Phase 3: Content Area Theming
**Status: Complete**

Theme colors applied to content rendering pipeline.

- Directory listings: text, links, search, error, external items colored by type
- Text pages: themed background and foreground colors
- Monochrome dark mode via srcBic/PaintRect (flicker-free white-on-black)
- Color rendering via RGBForeColor/RGBBackColor on Color QuickDraw systems
- Selection rendering: themed sel_bg/sel_fg on color, InvertRect on mono
- Offscreen buffer skipped on color systems (Phase 1 — direct drawing)

### Phase 4: UI Integration
**Status: Complete**

Theme submenu and preferences persistence.

- Options > Theme hierarchical submenu with 9 themes
- Separator divides monochrome (Light/Dark) from color themes
- Color themes dimmed on monochrome systems (Apple HIG compliant)
- Theme preference saved and restored across sessions
- Theme changes redraw all open windows

### Phase 5: Build System
**Status: Complete**

Build presets and memory budget for color support.

- Full preset: GEOMYS_COLOR=ON, GEOMYS_THEMES=ON, 768KB preferred
- Lite preset: GEOMYS_COLOR=OFF, GEOMYS_THEMES=ON (mono themes only)
- Minimal preset: both OFF
- `--themes`/`--no-themes` CLI flags

---

## Unreleased

### Text Page Scroll Optimization
**Status: Complete**

Extended ScrollRect fast-path scrolling from directory listings to text pages. Line index for O(1) row access, optimized row counting.

- `text_lines[]` line index in GopherState: byte offsets built incrementally during receive, max 3000 lines
- `content_draw_text_row()`: single-row text drawing with O(1) offset lookup (analogous to `content_draw_row()`)
- ScrollRect fast path extended to PAGE_TEXT: line scroll redraws 1-2 rows (~15x faster)
- `count_rows()` for text: O(1) return of `text_line_count` instead of O(N) byte scan
- Line index cached/restored in `cache.c`

### Text Selection & Copy
**Status: Complete**

Clipboard support for content area and address bar. Edit menu wired to focus context.

- Feature flag `GEOMYS_CLIPBOARD` with `clipboard.c`/`clipboard.h` module
- Focus tracking between address bar and content area
- Address bar: Cut/Copy/Paste via TEHandle and Scrap Manager (`TECut`, `TECopy`, `TEPaste`)
- Content area selection: click-drag, double-click word select, shift-click extend
- Selection state per row with inverse highlight rendering
- Copy selected text to system clipboard via `ZeroScrap`/`PutScrap`
- Edit menu enable/disable based on active focus and selection state
- I-beam cursor in content area, clear selection on page navigation and deactivate

---

## v0.9.0 — Type Handlers

### Phase 1: Binary File Downloads
**Status: Complete**

Streaming binary file downloads to disk via SFPutFile with progress display.

- PAGE_DOWNLOAD mode: streams received data directly to disk via FSWrite (near-zero memory overhead)
- SFPutFile dialog shown before network connection (System 6 and System 7 dual-path)
- Default filename derived from Gopher selector path (last component, sanitized, max 31 chars)
- File type/creator codes mapped per Gopher type (BinHex→TEXT/ttxt, Sound→sfil/SCPL, RTF→TEXT/MSWD)
- Download progress in status bar ("Downloading... N bytes")
- Cmd-. cancel with partial file cleanup
- Error handling: disk full detection, connection timeout warnings
- Types supported: 4 (BinHex), 5 (DOS binary), 6 (UUEncoded), 9 (binary), d (document)

### Phase 2: Image Metadata & Save
**Status: Complete**

Image header sniffing with format/dimension detection and save-to-disk.

- PAGE_IMAGE mode: buffers first 26 bytes to detect GIF/PNG format, then streams remainder to disk
- GIF header parsing: magic detection (GIF87a/GIF89a), little-endian width/height at bytes 6-9
- PNG header parsing: 8-byte magic, big-endian IHDR dimensions at bytes 16-23
- 68000-safe byte-level reads (no unaligned memory access)
- Image metadata displayed in status bar after save (format, dimensions, file size)
- File type/creator: GIF→GIFf/8BIM, PNG→PNGf/8BIM, unknown→????/8BIM
- Types supported: g (GIF), I (generic image), p (PNG)

### Phase 3: HTML URL Extraction
**Status: Complete**

Proper handling of type h (HTML) items with URL extraction and display.

- `URL:` prefix in selector: extracts URL, shows in copyable dialog
- `GET /` prefix in selector: builds HTTP URL from host/port/path, shows in dialog
- Bare HTML selectors: fetched and displayed as plain text (PAGE_TEXT)
- HTML type changed from navigable to explicitly handled in click paths
- HTML URL dialog with explanatory text and OK button

### Phase 4: Remaining Types
**Status: Complete**

Sound, RTF, and error type handling.

- Sound (type s): download to disk via PAGE_DOWNLOAD, sfil/SCPL type/creator
- RTF (type r): download to disk via PAGE_DOWNLOAD, TEXT/MSWD type/creator
- Error (type 3): fetched and displayed as PAGE_TEXT with "Server Error" status
- Telnet types (8, T): unchanged, show informational message

### Phase 5: Download UX & Browser Chrome
**Status: Complete**

Download progress dialog, visual metadata, navigation improvements.

- Download progress dialog: movable modal with filename, live byte count, Stop button
- Download-specific visual indicators: angle bracket labels (`<BIN>` vs `[TXT]`), distinct theme color, status bar hover hints
- `gopher_type_is_download()` centralized helper function
- Combined stop/go/refresh action button right of address bar
- Go menu: Back (Cmd-[), Forward (Cmd-]), Home, Refresh (Cmd-R), Stop (Cmd-.), Open Location (Cmd-L), browsing history
- History list moved from Window menu to Go menu
- Refresh no longer pushes to navigation history
- MacTCP pre-loaded at startup with status feedback
- Dynamic directory item array: 64 initial, grows to 2,000 max
- Previous page preserved during downloads (directory stays visible)
- Address bar cursor flicker fix

---

## v0.10.0 — HTML Renderer & Telnet Handoff

### Phase 1: HTML Tag-Stripping Renderer
**Status: Complete**

Single-pass streaming HTML tag stripper for type h items with bare selectors.

- 5-state parser state machine: TEXT, TAG_OPEN, TAG_CLOSE, ENTITY, SKIP
- Parser state (~32 bytes) added to GopherState under `#ifdef GEOMYS_HTML`
- Tag support: `<br>` newline, `<p>` blank line, `<pre>` preserve whitespace, `<h1>`–`<h6>` visual separation, `<li>` bullet, `<hr>` dash line
- `<script>` and `<style>` content skipped until closing tag
- Entity decoding: `&amp;` `&lt;` `&gt;` `&quot;`
- Whitespace collapsed outside `<pre>` blocks
- PAGE_HTML page type: tag-stripped content rendered via text display pipeline
- New files: `src/html.c`, `src/html.h`
- Feature-flagged with `GEOMYS_HTML`

### Phase 2: Telnet Connection Dialog
**Status: Complete**

Enhanced telnet connection dialog replacing NoteAlert for type 8/T items, with System 7 app launching.

- Movable modal dialog (DLOG 140): shows host, port, and login (if selector present)
- Copy Host button copies host:port to clipboard (guarded by `GEOMYS_CLIPBOARD`)
- Done button as default (rightmost, bold outline)
- Type T (TN3270) variant includes TN3270 mode note
- System 7 LaunchApplication: searches for Flynn or NCSA Telnet, launches with `launchContinue` flag
- System 6 fallback: dialog only (no launch capability)
- Status bar hover shows "Telnet: host:port" instead of "Telnet session (not supported)"
- Feature-flagged with `GEOMYS_TELNET`

---

## v0.11.0 — Performance & Polish

### Phase 1: Selection Drag Flash Optimization
**Status: Complete**

XOR delta rendering on monochrome eliminates flash during text selection drag. Pre-allocated clip region reduces Toolbox trap overhead.

- `invert_xor_delta()` helper: computes symmetric difference of old/new selection pixel ranges
- `sel_row_pixel_range()` helper: maps selection columns to pixel coordinates
- Monochrome path: `InvertRect` only on changed pixels (no erase/draw/invert cycle)
- Color path: clipped row redraws limited to changed rows
- Pre-allocated `RgnHandle` outside `StillDown()` loop
- Shadow buffer invalidated after drag for proper resync

### Phase 2: Double Buffering Refinements
**Status: Complete**

All scroll paths now use offscreen double buffering with partial CopyBits, eliminating flash on page/thumb scroll.

- Removed `g_scrolling` flag that bypassed offscreen during scroll
- Page/thumb vertical scroll uses offscreen buffer
- Horizontal scroll (action, thumb, scroll-to) uses offscreen buffer
- Partial CopyBits limits blit to dirty row union (≤20 rows)

### Phase 3: Local Page Cache Improvements
**Status: Complete**

Memory-aware dynamic cache sizing and weighted LRU eviction for smarter page caching.

- `FreeMem()` at init determines active slot count (3–8, capped at `CACHE_MAX`)
- Hit count field: frequently-accessed pages weighted higher in eviction score
- Multi-slot eviction: if allocation fails, evicts additional LRU slots before giving up
- Refresh (Cmd-R) verified to invalidate cache before re-fetching

### Phase 4: System 7 Polish
**Status: Complete**

System 7-specific improvements for better Finder integration and Apple Events support.

- `'vers'` resource (ID 1 and 2): version and description shown in Finder Get Info
- Apple Events `odoc` handler: opens files containing `gopher://` URLs
- About dialog version bumped to 0.11.0

---

## v0.12.0 — Icons, Menus & Async Networking

### Phase 1: CSO Phonebook Support
**Status: Complete**

Full CSO/ph phonebook protocol support for type 2 items.

- Query dialog with "Look Up" button, ph/qi protocol handling, formatted response display
- History integration preserves CSO queries for back/forward navigation
- Type 2 upgraded from informational stub to full interactive protocol support

### Phase 2: ROM Icons & List Manager
**Status: Complete**

Resource-based icons replace procedural drawing. List Manager for Favorites dialog.

- SICN resources (16x16): 11 Gopher type icons and 6 navigation button icons
- `cicn` color icons for all 17 SICN resources on Color QuickDraw systems
- SICN icons in menus: Home icon in Go menu, type-based icons on Favorites menu entries (System 7+)
- Navigation button icons: SICN resources replace ~185 lines of procedural QuickDraw polygon drawing
- List Manager for Favorites dialog: `LNew`/`LClick`/`LGetSelect` with keyboard type-ahead, native scrollbar, double-click to navigate
- Content-aware zoom: `calc_std_state()` sizes standard state from content width plus chrome

### Phase 3: Menu HIG Overhaul
**Status: Complete**

Menus restructured per Apple Human Interface Guidelines Chapter 4.

- File menu: "New Window" → "New", added "Open..." (Cmd-L), "Close Window" → "Close", "Save Page As..." → "Save As..."
- Edit menu: added "Show Clipboard" dialog
- Go menu streamlined to navigation only
- Options menu reordered: appearance first, configuration middle, view toggles last
- Font submenu split into separate Font and Size submenus for independent selection
- 8 fonts: Monaco, Geneva, Chicago, Courier, New York (plus Helvetica, Times, Palatino on System 7 via Gestalt)
- Size menu: independent point size selection (9, 10, 12, 14)
- Show/Hide menu toggles use `SetMenuItemText` for HIG-compliant dynamic text labels

### Phase 4: System 7 Polish
**Status: Complete**

Modernize System 7 APIs, Balloon Help, temporary memory, and larger screen support.

- Color QuickDraw detection via `Gestalt(gestaltQuickdrawVersion)` with `SysEnvirons` fallback
- Dynamic window sizing from `qd.screenBits.bounds` (512x342 minimum enforced)
- Multi-monitor drag/resize via `GetGrayRgn` bounding box
- Balloon Help: `hmnu` resources for all 7 menus with HIG-compliant help text
- `cache_alloc()`/`cache_free()` helpers use `TempNewHandle` on System 7, `NewPtr` fallback on System 6
- Print Apple Event (`kAEPrintDocuments`) for Finder File > Print support
- Removed TSM flag and orphaned DLOG 131 resource

### Phase 5: Scroll & Rendering Fixes
**Status: Complete**

Fix stale content, theme flash, and scroll rendering issues.

- Content area blanks immediately on navigation (eliminates ~2s stale content)
- Theme switching erases to new background in one frame (no progressive flash)
- Horizontal scroll position resets on font/size change
- Horizontal scroll uses `ScrollRect` pixel-shifting (was full `content_draw`)
- Line-scroll exposed rows wrapped in offscreen double buffer

### Phase 6: Async TCP Connect & DNS Cache
**Status: Complete**

Non-blocking TCP connections keep the UI responsive during handshakes. Multi-entry DNS cache accelerates repeat visits.

- `_TCPActiveOpen` called asynchronously with idle-loop polling via `conn_connect_poll()`
- Selector send deferred until connection completes, stored in per-session `send_selector` buffer
- `CONN_STATE_OPENING` state for async handshake in progress
- Timeout detection during async handshake
- 8-entry LRU DNS cache: stores hostname, IP, TTL-based expiry, and last access time
- TTL extracted from DNS A record response, clamped to 5min–1hr
- Back/forward and multi-server browsing skip DNS resolution for cached hosts

---

## v0.13.0 — Gopher+ Get Info & Prefs Icon

### Phase 1: Preferences Document Color Icons
**Status: Complete**

Added missing color icon resources for the preferences document type (resource 129).

- `icl4` (129): 32x32 4-bit color icon (tan paper, black border/text, gray fold)
- `icl8` (129): 32x32 8-bit color icon
- `ics4` (129): 16x16 4-bit color icon
- `ics8` (129): 16x16 8-bit color icon
- Matches Flynn's complete icon suite pattern (ICN#, ics#, icl4, icl8, ics4, ics8)
- FREF/BNDL structures verified correct (no changes needed)

### Phase 2: Gopher+ Get Info Dialog
**Status: Complete**

Full Gopher+ attribute fetch and display for items on Gopher+ servers.

- File menu: "Get Info..." (Cmd+I) at position 5, enabled on directory pages with keyboard-selected row
- DLOG/DITL 143: movableDBoxProc dialog with "Done" button, 8 static text fields (display name, type, server, selector, admin, modified, views)
- `GopherPlusInfo` aggregate struct for complete attribute response
- `gopherplus_parse_response()`: scans for +ADMIN and +VIEWS blocks in attribute data
- `gopherplus_fetch_info()`: synchronous TCP fetch using `selector\t!` request, 10-second timeouts, yields via WaitNextEvent
- `gopher_type_label()`: human-readable names for all 18 Gopher types
- Status window shown during attribute fetch
- HIG-compliant dialog layout: 13px margins, "Done" button (not "OK"), default button outline

---

## v0.14.0 — System 7 Polish

### Phase 1: Stationery & Notifications
**Status: Complete**

Stationery pad support and improved background page-load notifications.

- SIZE resource: `isStationeryAware` flag for stationery pad template files
- Notification Manager: SICN icon handle (`nmIcon`) and descriptive Pascal string (`nmStr`) for background page-load alerts on System 7

### Phase 2: AppleScript Support
**Status: Complete**

Scriptable vocabulary for automation on System 7+.

- `aete` resource (ID 0): "Geomys Suite" (`GEOM`) with level 1, version 1
- Event `navigate` (`GEOM`/`GURL`): navigate to a `gopher://` URL (direct parameter, typeChar)
- Event `get URL` (`GEOM`/`gURL`): return current page URL as text (reply parameter, typeChar)
- `AEInstallEventHandler` calls Gestalt-gated for System 7+

### Phase 3: Drag Manager
**Status: Complete**

URL drag-and-drop via Drag Manager on System 7.5+.

- Drag OUT: drag a link from content area to Desktop or other apps — creates text clipping with `TEXT` flavor
- Drag IN: drop a text clipping or URL into any Geomys window — navigates if `gopher://` prefix detected
- Gestalt-gated via `gestaltDragMgrAttr`; no-op on System 6/7.0–7.1
- New files: `src/drag.c`, `src/drag.h`
- Feature flag: `GEOMYS_DRAG` with conditional compilation
- Drag Manager trap bindings via `M68K_INLINE` dispatch (`0xABED`)
- Tracking handler highlights drop target; receive handler extracts URL and navigates

---

## v0.15.0 — Gopher+ Protocol Suite

### Phase 1: +ABSTRACT Attribute
**Status: Complete**

Parse and display item abstracts from Gopher+ servers.

- `+ABSTRACT` block parsing in `gopherplus_parse_response()`
- Abstract text stored in `GopherPlusInfo` struct (truncated to fit)
- 3-line scrollable abstract field in Get Info dialog (DLOG 143)
- Graceful truncation for long abstracts
- Guarded by `GEOMYS_GOPHER_PLUS` feature flag

### Phase 2: Search Result Scoring
**Status: Complete**

Display relevance scores for Gopher+ search results.

- `+SCORE` block parsing from search result attributes
- Inline score extraction from Gopher+ status lines
- `[ 85]` right-aligned score display in Traditional page style
- Score column only shown when search results contain scoring data
- Guarded by `GEOMYS_GOPHER_PLUS` feature flag

### Phase 3: Bulk Attribute Fetch ($)
**Status: Complete**

Single-request attribute retrieval for all items in a directory listing.

- `selector\t$` bulk attribute request on Gopher+ directories
- 16-entry compact attribute cache (~3.2 KB) per directory
- Sequential cursor for item matching (no nested linear scan)
- Lazy fetch: triggered on first Get Info request for a directory
- Hover abstracts displayed in status bar from cached attributes
- Guarded by `GEOMYS_GOPHER_PLUS` feature flag

### Phase 4: Content Negotiation (+VIEWS)
**Status: Complete**

Request alternate content representations from Gopher+ servers via +VIEWS data.

- "Choose View..." button in Get Info dialog for items with multiple views
- View selection dialog (DLOG 144): list of available content types
- Maximum 2 stacked modals (HIG limit for modal depth)
- Gopher+ status line (`+`) handling on content responses
- Integration with existing download and display pipelines
- Guarded by `GEOMYS_GOPHER_PLUS` feature flag

### Phase 5: +ASK Form Support
**Status: Complete**

Interactive Gopher+ forms with programmatic dialog generation.

- `+ASK` block parsing: Ask (text), AskP (password), AskL (multiline), Select (checkbox), Choose (radio)
- Programmatic dialog via `NewDialog()` with dynamic DITL construction
- Password field masking for AskP items
- Form response submission as tab-delimited POST body
- Maximum 8 fields per form
- Heap-allocated form struct (never stack — 68000 stack budget)
- Guarded by `GEOMYS_GOPHER_PLUS` feature flag

---

## v0.15.1 — Code Review Remediation
**Status: Complete**

Security, performance, memory, and code quality improvements from code review.

- Port number validation in `gopher_parse_line()`: clamp to 0–65535 range
- Batched `html_emit_run()` for HTML text: single `DrawText` call instead of per-character
- Cached `visible_rows()` result: computed once per draw pass
- Reuse `TextWidth` result for hover underline width
- Eliminated `send_selector` duplication: reuse `cur_selector` in `GopherState` (~256 bytes saved)
- Extracted helpers: `gopher_grow_text_buf()`, `gopher_grow_text_lines()`, `content_draw_selection()`
- Deduplicated Apple Event document handler and `browser.c` color restore logic

---

## Future Features

### Potential v1.0 — Release Candidate

Polish pass, QA, and final release.

- Final documentation pass and release notes
- Version bump to 1.0.0 in all artifacts
- Build all 3 editions (Full, Lite, Minimal)
- Tag release and publish to Forgejo + GitHub mirror

#### Acceptance Criteria

- All 19 canonical and non-canonical Gopher item types tested against
  live servers (sdf.org and others)
- All 3 build editions (Full, Lite, Minimal) compile and produce
  working .dsk floppy images and .hqx archives
- System 6.0.8 testing in Snow emulator (Mac Plus, monochrome)
- System 7.x testing in Snow emulator (Mac Plus and Mac II profiles)
- Gopher+ Get Info, content negotiation, and forms verified against
  a Gopher+ server
- Multi-window browsing: open 4 windows, background loading,
  close/reopen cycle without memory leaks
- All keyboard shortcuts functional on M0110 and M0110A keyboards
- Preferences persist across launch/quit cycles on System 6 and 7
- Color themes render correctly on 256-color systems; monochrome
  themes render correctly on Mac Plus

#### QA Checkpoints

- [ ] Fresh install: copy to blank hard drive image, launch, navigate
- [ ] Home page: set, verify on relaunch, clear, verify blank
- [ ] Navigation: back/forward/refresh/home/stop through 5+ pages
- [ ] Search (type 7): query dialog, results, back to results
- [ ] CSO (type 2): phonebook lookup
- [ ] Downloads: binary, image, sound — save to disk, verify files
- [ ] HTML (type h): URL extraction dialog, tag-stripping renderer
- [ ] Telnet (type 8/T): connection info dialog, Copy Host
- [ ] Favorites: add, edit, delete, reorder, menu quick-access
- [ ] Find in Page: search, Find Again, wrap around
- [ ] Print: Page Setup, Print to ImageWriter/LaserWriter driver
- [ ] Gopher+: Get Info, Choose View, Fill Form, bulk attributes
- [ ] Multi-window: open 4, navigate independently, close all
- [ ] Themes: cycle all 9, verify color and monochrome rendering
- [ ] Fonts/sizes: all 8 fonts at all 4 sizes
- [ ] Page styles: Traditional, Plain, Markdown
- [ ] Stationery: create stationery document, open it (System 7)
- [ ] AppleScript: navigate and get URL commands (System 7)
- [ ] Drag Manager: drag link out, drop URL in (System 7.5)
- [ ] Stress: 2,000-item directory, large text page, rapid navigation

#### Known Limitations (Deferred to v2.0)

- Preferences document icon does not appear on System 6: this is a
  Finder limitation, not a code bug — the icon resources are correctly
  defined and display properly on System 7 and later
- No contextual menus: requires System 7+ Contextual Menu Manager,
  out of scope for System 6 primary target
- Single-level undo in address bar: only one undo/redo step is
  supported; multi-level undo deferred
- Edit menu does not show action names (e.g., "Undo Typing"):
  requires TextEdit action tracking, deferred to v2.0
