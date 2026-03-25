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

### Phase 6: Async TCP Connect
**Status: Complete**

Non-blocking TCP connections keep the UI responsive during handshakes.

- `_TCPActiveOpen` called asynchronously with idle-loop polling via `conn_connect_poll()`
- Selector send deferred until connection completes, stored in per-session `send_selector` buffer
- `CONN_STATE_OPENING` state for async handshake in progress
- Timeout detection during async handshake

---

## Future Features

(None planned)
