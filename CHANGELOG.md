# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased] — Menu HIG Overhaul

### Changed
- File menu restructured per Apple HIG Chapter 4: "New Window" → "New", added "Open..." (Cmd-L, focuses address bar), "Close Window" → "Close", "Save Page As..." → "Save As...", reordered with standard separator grouping
- Edit menu: added "Show Clipboard" item with dialog displaying current clipboard text content (up to 253 characters)
- Go menu streamlined to navigation only: removed "Open Location..." (moved to File > Open...)
- Options menu reordered per HIG: appearance settings first (Font, Size, Page Style, Theme), configuration middle (Home Page, DNS Server), view toggles last (Show/Hide Details, Show/Hide Status Bar)
- Font submenu split into separate Font and Size submenus for independent selection
- Font menu lists font names only (Monaco, Geneva, Chicago, Courier, New York); Helvetica, Times, and Palatino appended dynamically on System 7 via Gestalt gate
- Size menu provides independent point size selection (9, 10, 12, 14)
- Balloon Help (`hmnu`) resources updated for all restructured menus and new Size submenu

## [Unreleased] — ROM Icons & List Manager

### Added
- SICN resources (16x16): 11 Gopher type icons (IDs 256–266: folder, document, search, error, binary, terminal, image, globe, speaker, phonebook, unknown) and 6 navigation button icons (IDs 270–275: back, forward, home, stop, go, refresh)
- `cicn` color icons for all 17 SICN resources on Color QuickDraw systems: folder blue, error red, terminal green, globe teal, image purple, speaker magenta, nav buttons in themed colors; lazy-cached via `GetCIcon` with `PlotCIcon` drawing
- SICN icons in menus: Home icon in Go menu, type-based icons on Favorites menu entries (System 7+ only, via `SetItemIcon`/`SetItemCmd(0x1E)` per Inside Macintosh)
- List Manager for Favorites dialog: `LNew`/`LClick`/`LGetSelect` replace custom list drawing; adds keyboard type-ahead, native scrollbar, double-click to navigate, and proper `LUpdate` via UserItem draw proc
- Content-aware zoom: `calc_std_state()` sets `WStateData->stdState` from content width plus chrome (scrollbar, borders), clamped to screen bounds with 200px minimum

### Changed
- Gopher type icons: 16x16 SICN resources loaded via `GetResource('SICN')` for fonts with row height ≥ 16; original 11x11 procedural bitmaps retained for Monaco 9
- Navigation button icons: SICN resources replace ~185 lines of procedural QuickDraw polygon drawing (`OpenPoly`/`LineTo`/`PaintPoly`); disabled state uses standard `patBic` gray overlay
- Color icon fallback: `gopher_cicn_draw()` attempts `cicn` first on color systems, falls back to monochrome SICN automatically
- Show/Hide menu toggles: Options menu "Show Details"/"Hide Details" and "Show Status Bar"/"Hide Status Bar" use `SetMenuItemText` for HIG-compliant dynamic text labels (no checkmarks)
- Favorites menu entries: duplicate SICN 289 for folder icon (SICN 256 maps to icon number 0 which means "no icon")

## [Unreleased] — System 7 Polish

### Added
- Balloon Help: `hmnu` resources for all 7 menus (Apple, File, Edit, Go, Favorites, Options, Window) with HIG-compliant help text
- Temporary memory for cache: `cache_alloc()`/`cache_free()` helpers use `TempNewHandle` for cache slot allocations on System 7, keeping the app heap free for working data; falls back to `NewPtr` on System 6
- TSM input method support: `useTextEditServices` enabled in SIZE resource

### Changed
- Color QuickDraw detection: `Gestalt(gestaltQuickdrawVersion)` preferred over `SysEnvirons()`, with `SysEnvirons` fallback for pre-6.0.4 systems
- Dynamic window sizing: windows sized to actual screen dimensions via `qd.screenBits.bounds` instead of hardcoded 512x342 (Mac Plus minimum enforced)
- Status window centering: `conn_status_show()` uses `qd.screenBits.bounds` instead of hardcoded 512x342 coordinates
- Multi-monitor drag/resize: `DragWindow` and `GrowWindow` use `GetGrayRgn` bounding box (full desktop area) instead of `qd.screenBits.bounds` (main monitor only)

### Performance
- `TempNewHandle` guarded with System 7 version check via `Gestalt` — cache operates safely on System 6 without MultiFinder
- CSO phonebook entry tracker (`cso_last_entry`) moved from file-global to per-session `GopherState` field — fixes multi-window CSO response boundary corruption

### Memory
- Cache `text_lines` allocation right-sized to `max(actual * 2, GOPHER_INIT_TEXT_LINES)` instead of always allocating `GOPHER_MAX_TEXT_LINES` — saves up to 10KB per small cached page

### Code Quality
- Extracted `dismiss_modal()` helper, deduplicating 8 dialog dismiss + window invalidate call sites
- Extracted `restore_title_bar()` helper, deduplicating 7 title-restore-from-history call sites
- Fixed C89 declaration ordering in `do_search_dialog` and `do_cso_dialog`

## [Unreleased] — CSO Phonebook Support

### Added
- CSO/ph phonebook queries (type 2): query dialog with "Look Up" button, ph/qi protocol handling, formatted response display
- History integration preserves CSO queries for back/forward navigation

### Changed
- Type 2 (CSO) upgraded from informational stub to full interactive protocol support
- Documentation updated: item type support now accurately described by tier (interactive, download, telnet handoff, display) instead of blanket "fully supported"

## [0.11.2] — Internal: Code Review Improvements

### Performance
- Address bar clip region pre-allocated once at init instead of `NewRgn()`/`DisposeRgn()` per draw cycle
- Content rect and theme pointer passed directly to row-draw functions, eliminating redundant per-row recomputation and `theme_current()` calls
- Manual string formatting in `poll_active_session()` data-receive path replaces `snprintf` overhead
- Simplified DNS transaction ID entropy collection with fewer Toolbox trap calls

### Memory
- Cache text buffer sized to actual content length plus headroom on retrieve, instead of always allocating 32KB

### Security
- `FSRead` return value checked in Apple Event `odoc` handler — prevents navigating to garbage data on read failure
- `sprintf` replaced with `snprintf` in save-file path construction (defensive bounds checking)
- HTML numeric entity parser guards against `short` overflow on malformed `&#nnnnn;` sequences

### Code Quality
- Consolidated `do_download_file()` and `do_image_save()` into shared helpers, reducing ~150 lines of duplicated save logic
- Extracted `handle_download_completed()` and `handle_image_completed()` from `handle_page_loaded()`, splitting a large function into focused handlers
- Extracted theme row-erase helper, deduplicating background setup across draw paths

## [0.11.1] — Internal: Performance, Memory & Security Hardening

### Performance
- Row formatting: extracted `format_row_text()` using `memcpy` for fixed prefixes instead of `snprintf`, avoiding format-string overhead on 68000
- Text ingestion: bulk `memchr`/`memcpy` in `gopher_process_data()` replacing byte-at-a-time processing, with fast trailing-byte check for `\r\n` stripping
- Draw loop: cache `theme_current()` once per row draw instead of 4 separate calls
- Draw loop: cache `sel_normalize()` once before the draw loop instead of up to 4 times per row
- Draw loop: replace 512-byte `g_dirty[]` scan with `g_dirty_count` counter, eliminating O(512) scan on every draw pass
- Selection rendering: pass pre-formatted row text to `draw_selection_rect()` from `content_draw_row()`, avoiding redundant `content_row_text()` call

### Memory
- Growable text buffers: start at 8KB / 512 line entries, grow to 32KB / 3000 max on demand — saves ~24KB per small text page
- Cache guard: skip `cache_store()` when `FreeMem()` < 200KB, preventing cache from consuming the last available memory on a 4MB Mac Plus
- Pre-allocate reusable clip region (`g_clip_rgn`) in `content_init()` instead of `NewRgn()`/`DisposeRgn()` per row per draw pass
- Reduce `dns_cache_host` from 256 to 80 bytes — Gopher hostnames are limited to 64 bytes in `GopherItem`

### Security
- Harden DNS `txn_id` with additional entropy sources (mouse position, `FreeMem`, counter with Knuth multiplicative hash) to resist spoofing

### Code Quality
- Extract `set_item_fg_color()` helper, replacing 3 duplicated copies of item-type-to-foreground-color selection logic
- Extract `shadow_needs_draw()`/`shadow_update()` helpers to deduplicate shadow buffer comparison logic
- Extract `finish_download()` helper, sharing cleanup code between `PAGE_DOWNLOAD` and `PAGE_IMAGE` completion paths
- Extract `poll_active_session()`/`poll_all_sessions()` helpers, replacing ~200 lines of inline null event polling code
- Extract `browser_begin_addr_clip()`/`browser_end_addr_clip()` helpers, deduplicating address bar text-width clipping pattern
- Net reduction of ~63 lines through helper extraction and deduplication

## [0.11.0] — Performance & Polish

### Added
- `'vers'` resource for System 7 Finder Get Info display
- Apple Events `odoc` handler: open files containing `gopher://` URLs (drag-and-drop onto app icon)
- Memory-aware cache sizing: active slot count scales with available RAM (3–8 slots via `FreeMem()`)
- Weighted LRU cache eviction: frequently-visited pages (hit count) stay cached longer
- Multi-slot cache eviction: if allocation fails, evicts additional slots before giving up

### Changed
- Selection drag rendering: XOR delta on monochrome — only inverts changed pixels instead of full row redraws, eliminating flash during text selection drag
- Selection drag rendering: pre-allocated clip region avoids `NewRgn`/`DisposeRgn` per row per mouse event
- Color selection drag: clipped row redraws limited to changed rows (unchanged rows skipped)
- Page/thumb scroll now uses offscreen double buffering (was previously skipped), eliminating flash on page scroll
- Horizontal scroll now uses offscreen double buffering
- Removed `g_scrolling` flag — all scroll paths now use offscreen rendering with partial CopyBits

### Fixed
- Shadow buffer invalidated after selection drag to prevent stale state on next `content_draw()`

## [0.10.0] — HTML Renderer & Telnet Handoff

### Added
- HTML tag-stripping renderer: single-pass streaming parser strips tags and renders as plain text (PAGE_HTML)
- HTML entity decoding: `&amp;` `&lt;` `&gt;` `&quot;`
- HTML tag support: `<br>`, `<p>`, `<pre>`, `<h1>`–`<h6>`, `<li>`, `<hr>`, `<a>` with URL extraction
- HTML `<script>` and `<style>` content skipped entirely
- Whitespace collapse outside `<pre>` blocks
- `GEOMYS_HTML` feature flag with `html.c`/`html.h` module
- Telnet connection dialog: movable modal (DLOG 140) showing host, port, and login for type 8/T items
- Copy Host button in telnet dialog copies host:port to clipboard
- System 7 LaunchApplication: launch Flynn or NCSA Telnet with `launchContinue` flag
- `GEOMYS_TELNET` feature flag with conditional compilation

### Changed
- Type h (HTML) bare selectors now rendered with tag stripping instead of raw plain text
- Type 8/T (Telnet/TN3270) replaced NoteAlert with enhanced connection info dialog
- Status bar hover for telnet items shows "Telnet: host:port" instead of "Telnet session (not supported)"

## [0.9.0] — Type Handlers & Downloads

### Added
- Binary file downloads: save types 4 (BinHex), 5 (DOS binary), 6 (UUEncoded), 9 (binary), d (document) to disk via SFPutFile
- Image save with metadata: types g (GIF), I (image), p (PNG) — shows format, dimensions, and file size after save
- GIF and PNG header parsing for image dimensions (68000-safe byte-level reads)
- HTML URL extraction: type h with `URL:` prefix displays URL in copyable dialog
- HTML `GET /` prefix support for legacy Gopher-to-HTTP links
- Bare HTML selectors fetched and displayed as plain text
- Sound file download (type s) and RTF document download (type r)
- Download progress dialog: movable modal with filename, live byte count, and Stop button
- Download cancel via Cmd-. or Stop button with partial file cleanup
- Suggested filenames derived from Gopher selector path
- File type/creator code mapping for all download types
- `gopher_type_is_download()` helper for centralized download type detection
- Download-specific `link_download` theme color (teal/cyan) across all 9 themes
- Angle bracket labels for download types: `<BIN>` vs `[TXT]` in Traditional page style
- Status bar hover hints: shows gopher:// URI for navigable items, "click to save to disk" for downloads, extracted URL for HTML links
- Hand cursor on download items (previously only navigable types)
- Combined Stop/Go/Refresh action button right of address bar: stop icon during loading, go arrow when URL edited, refresh when idle
- Go menu with Back (Cmd-[), Forward (Cmd-]), Home, Refresh (Cmd-R), Stop (Cmd-.), Open Location (Cmd-L), and browsing history list
- MacTCP pre-loaded at startup with "Loading MacTCP..." status and watch cursor
- Dynamic directory item array: starts at 64 items, grows to 2,000 max (was fixed at 200)

### Changed
- Type 3 (Error) items now fetch and display server error text instead of generic message
- Navigation buttons reduced to Back, Forward, Home (Refresh/Stop replaced by action button)
- Address bar wider — extends between nav buttons and action button
- History list moved from Window menu to Go menu
- Refresh no longer pushes to navigation history
- Download completion treats server close as success (Gopher has no Content-Length)
- Previous page preserved during downloads — directory listing stays visible
- Home Page dialog default cleared (was pre-filled with sdf.org)
- Directory item array uses 67% less memory for typical pages (26KB vs 80KB)

### Fixed
- Type h (HTML) no longer attempts to parse HTML as Gopher directory
- HTML URL extraction checks selector field (not display field) for `URL:` prefix
- Page rendering after load: force full redraw to prevent invisible rows from shadow buffer staleness
- SFPutFile dialog ghost: window redrawn immediately after save dialog dismisses
- Download state preserved: address bar, cache, and history stay correct during downloads
- Action button updates to refresh after download/cancel completes
- Address bar I-beam cursor no longer flickers during typing
- Lite build: download types correctly non-navigable when GEOMYS_DOWNLOAD is OFF

## [0.8.0] — HIG Improvements

### Added
- Watch cursor during page loads: watch cursor shown while connecting, resolving, and receiving data
- Tab key focus cycling: Tab/Shift-Tab cycles between address bar and content area
- Keyboard link navigation: Up/Down arrows select navigable links on directory pages, Return follows selected link
- Focus ring (gray dotted outline) on keyboard-selected row
- Window cascade positioning: new windows offset diagonally instead of stacking directly on top
- Address bar undo/redo: single-level Undo (Cmd-Z) for address bar text, toggles to Redo after undo
- Undo/Redo menu label toggles dynamically per HIG p.113
- Loading progress indicator: status bar shows live "Loading... N items" or "Loading... N bytes" during page fetch
- Improved error messages: connection errors now include the hostname, suggested fixes, and use Mac curly quotes
- Timeout status: "Connection timed out" shown in status bar when receive timeout fires
- History list: last 10 visited pages with checkmark on current page (moved to Go menu in 0.9.0)
- Direct history navigation: click any history entry to jump to it
- Print support: Page Setup and Print (Cmd-P) in File menu for printing current page content
- `GEOMYS_PRINT` feature flag with conditional compilation

### Changed
- Default home page changed to blank (was `gopher://sdf.org`); users can set their own via Options > Home Page
- File menu reorganized: Page Setup and Print added between Save Page As and Close Window
- New window memory error messages include actionable guidance ("Try closing other windows or applications")
- Save file error messages include probable cause ("The disk may be full or locked")
- DNS and connection errors include the hostname and suggest checking the address or network

## [0.7.0] — Find in Page & Keyboard Shortcuts

### Added
- Find in Page (Cmd-F): case-insensitive text search across directory listings and text pages
- Find Again (Cmd-G): repeat search from last match, wraps around
- Find dialog pre-fills previous search query
- Match highlighting using selection system
- Cmd-L: focus address bar and select all text (classic Netscape "Open Location")
- Cmd-R: refresh/reload current page

## [0.6.1] — Loading Regression Fixes

### Fixed
- Window title bar stuck on "Loading..." after navigation failure — title now properly restored to previous page
- Search dialog (Type 7) not recovering from failed navigation — app state and title now properly reset
- Added 30-second connection receive timeout to prevent indefinite loading hangs on unresponsive servers
- TCP send selector failure now properly detected and reported instead of silently hanging
- Increased SIZE resource for multi-window color builds to prevent memory exhaustion after several navigations

## [0.6.0] — System 7 Enhancements

### Added
- NewCWindow for color systems: proper CGrafPort on Mac II+ (NewWindow fallback on B&W)
- GWorld color offscreen buffer for flicker-free rendering on color displays (~171KB at 8-bit)
- Notification Manager: diamond mark and system alert sound when background page load completes (System 7)
- Movable modal dialogs on System 7 (Search, Open URL, Home Page, Favorites, Edit Favorite)
- Dialog auto-centering on screen (System 6 and System 7)
- Color icon family for Finder: icl4, icl8, ics4, ics8 resources
- Dirty-row tracking: skip unchanged rows during redraw
- Shadow buffer: detect identical row content to avoid redundant drawing
- System chrome colors via GetAuxWin: reads window color table for chrome areas
- Font metrics caching per session (avoids CharWidth recomputation on window switch)
- Background session throttling for idle windows
- "Can't Undo" menu item (dimmed) per Apple HIG p.113

### Changed
- Key repeat tuning: 200ms initial delay, 33ms repeat (~30 cps) for snappier keyboard navigation
- Process chunking: batch TCP reads with 4-tick draw deadline for faster page loads
- Partial CopyBits: blit only dirty row regions instead of full content area
- Selection highlighting uses theme sel_bg/sel_fg colors for proper inverse theming on color
- Selection drag rendering: XOR delta eliminates monochrome flash during text selection
- BrowserSession struct field ordering optimized for 68000 d16(An) addressing

### Fixed
- GWorld offscreen buffer cleared on allocation and resize (prevents pixel corruption)
- Shadow buffer invalidated on resize, theme switch, font change, and system update events
- Tokyo Light theme: removed purple tint from background color (neutral gray)
- Platinum theme: lightened chrome background to match content area
- Key repeat settings restored to system defaults on quit

## [0.5.1] — System 7 Performance

### Added
- DNS hostname caching for faster repeated navigation to the same server

### Fixed
- Window close performance on System 7 (replaced blocking TCPClose with immediate TCPAbort)
- Missing double buffering on System 7 (offscreen buffer was incorrectly disabled when Color QuickDraw detected)
- SIZE resource flags for proper System 7 Process Manager integration (isHighLevelEventAware)

### Changed
- Drawing performance: removed redundant DrawGrowIcon, region, and color operations per frame
- MultiFinder cooperation: tuned WaitNextEvent sleep times and CPU yielding during connections
- Memory Manager initialization: MaxApplZone before Toolbox init, pre-allocated master pointers

## [0.5.0] — 256-Color Themes

### Added
- 256-color support with runtime Color QuickDraw detection
- 9 built-in themes: Light, Dark, Solarized Light/Dark, Tokyo Night Light/Dark, Green Screen, Classic, Platinum
- Options > Theme submenu for theme selection with persistent preference
- Monochrome themes (Light/Dark) work on all systems including Mac Plus
- Color themes automatically available on Mac II and later with Color QuickDraw
- Dark mode: flicker-free white-on-black rendering via srcBic transfer mode
- Themed content colors by Gopher item type (text, links, search, errors, external)
- Color caching for efficient RGBForeColor/RGBBackColor trap calls on 68000
- `GEOMYS_COLOR` and `GEOMYS_THEMES` feature flags with build presets
- Chrome theming stubs for future nav bar and status bar color support

### Changed
- Full build preset: GEOMYS_COLOR=ON by default, 768KB preferred partition
- Lite build preset: monochrome themes only (Light/Dark)
- Offscreen double buffer skipped on color systems (direct drawing)
- Selection rendering: themed colors on Color QuickDraw systems
- Preferences version bumped to 4 (adds theme_id field)

## [0.4.0] — Horizontal Scrolling

### Added
- Horizontal scrollbar for content wider than the window
- Left/Right arrow keys scroll content horizontally (one character width per step)
- Up/Down arrow keys scroll content vertically (one row per step)
- Page Up/Page Down scroll vertically by one visible page
- Home key scrolls to top of document
- End key scrolls to bottom of document
- All navigation keys support key repeat for continuous scrolling
- Keyboard navigation is focus-aware: arrow keys move cursor when address bar is focused

### Fixed
- Width measurement parity with drawing code for Gopher+ indicators
- CP437 character translation applied during text page width measurement

## [0.3.0] — Multi-Window Browsing

### Added
- Multi-window browsing: open up to 4 simultaneous Gopher windows
- Window menu for switching between open windows
- New Window (Cmd-N) and Close Window (Cmd-W) commands
- Background loading: pages load in background windows while browsing
- Per-window history, scroll position, and text selection
- `GEOMYS_MAX_WINDOWS` build flag with `--max-windows N` CLI override
- Build presets scale window count: Full (4), Lite (2), Minimal (1)
- Window title shows "Loading host..." during page fetch
- Memory pressure alert when unable to open new window

### Changed
- File menu reorganized: New Window, Open URL, Save Page As, Close Window, Quit
- Close Window (Cmd-W) closes front window; closing last window quits application
- Cache pool shared across windows with per-session keying and scaled slot allocation (3/4/6 slots)
- Inactive window scrollbar dimmed per Apple HIG
- Offscreen buffer adapts bounds per-window and resizes on window grow
- Single-window builds (GEOMYS_MAX_WINDOWS=1) have zero multi-window overhead

## [0.2.1] — Performance & Polish

### Changed
- Scroll performance: line scroll uses ScrollRect to shift pixels, redraws only 1-2 rows (~15x faster)
- Hover performance: targeted row redraws instead of full page redraw (~12x faster)
- Page styles now functional: Traditional (type labels), Plain (underlined links), Markdown (bullet prefixes)
- CP437 character set translation integrated into text page rendering with fast ASCII bypass
- Gopher+ items show `+` suffix on type labels in Traditional style
- Grow box clip rect aligned consistently between update and content draw
- Cursor update skips redundant redraws when hover row unchanged

### Fixed
- Preferences document icon: corrected FREF local icon ID byte order (was `0100`, now `0001`)

## [0.2.0] — Polish Pass

### Added
- Offscreen double buffering for flicker-free rendering (feature-flagged)
- Font submenu: Monaco 9, Geneva 9, Chicago 12 (Options > Font)
- Hand cursor on navigable Gopher items
- CP437 character set translation for box-drawing and special characters
- Unicode glyph rendering engine (adapted from Flynn)
- Local page cache: 3-page LRU, instant back/forward navigation
- Search query preservation in navigation history
- Gopher+ protocol support: `has_plus` detection, `+ADMIN` and `+VIEWS` parsing
- Page format styles: Traditional, Plain, Markdown (Options > Page Style)
- New pocket gopher application icon

### Changed
- Build system: 10 feature flags with conditional compilation
- Build presets: minimal, lite, full (default changed from lite to full)
- Command-line flags for all features (`--flag`/`--no-flag` variants)
- CP437 auto-enables GLYPHS dependency
- Dynamic SIZE resource computed from enabled features
- Cosmetic fixes: grow box drawing, text clipping, navigation button icons

## [0.1.0] — MVP

### Added
- Project scaffolding: directory structure (`src/`, `include/`, `resources/`, `scripts/`, `docs/`)
- Build system: `CMakeLists.txt` with feature flags, `scripts/build.sh` with preset support
- Menu bar: Apple, File, Edit, Favorites, Options menus with standard keyboard shortcuts
- About Geomys dialog with version, build SHA, machine name, and copyright
- Event loop with WaitNextEvent, mouse and keyboard handling
- Utility modules adapted from Flynn: `macutil.c`, `sysutil.c`, `MacTCP.h`
- Rez resource file (`geomys.r`) with menus, dialogs, icons, SIZE, BNDL, and FREF
- Initial documentation: README.md, CHANGELOG.md, BUILD.md, JOURNAL.md, ROADMAP.md, About Geomys
- TCP/IP networking via MacTCP with 8KB receive buffer
- DNS resolver with UDP-first, TCP-fallback (adapted from Flynn)
- Gopher protocol engine implementing RFC 1436 line parser and GopherItem struct
- RFC 4266 URI parser (`gopher://host[:port]/type[selector]`)
- Connection state machine: IDLE → RESOLVING → CONNECTING → SENDING → RECEIVING → DONE/ERROR
- Directory listing display with click-to-navigate for linked items
- Type handler registry for all 18 Gopher types (canonical and non-canonical)
- Monaco 9 monospaced text rendering for content display
- Auto-navigate to `gopher://sdf.org` on launch
- Browser chrome: navigation bar with Back/Forward/Refresh/Home buttons
- Address bar with TextEdit field, Return to navigate, auto-updates on page load
- Status bar showing connection state (Ready, Loading, Done — N items)
- Open URL dialog (Cmd-L / File > Open URL) with pre-filled current URL
- Window title bar, close box, and resizable grow box
- Connection progress shown in status bar (removed modal dialog)
- Vertical scroll bar with line/page/thumb scrolling
- Content area clips to exclude scrollbar column
- Per-row erase to reduce scroll flicker
- Grow box persists after scroll and status bar updates
- Type 7 (Search) with query dialog, appends query to selector
- All 18 Gopher types handled with informational messages for unsupported types
- Error recovery: failed connections preserve previous page content
- ModalDialog loops for proper EditText interaction in dialogs
- Navigation history stack (10 entries, URL-only, re-fetch on navigate)
- Back/Forward buttons and Cmd-[/Cmd-] keyboard shortcuts
- History undo on failed back/forward navigation
- User-configurable home page (Options > Home Page, blank page option)
- Preferences persistence ("Geomys Preferences" file)
- Favorites system: Add (Cmd-D), Manage, Edit, Delete, Move Up/Down, Go To
- Top 5 favorites shown in Favorites menu
- Page titles from clicked item display names in window title bar
- Delete confirmation with Cancel as default (HIG safe action)
- Release script (`scripts/release.sh`) for Forgejo and GitHub
