# Changelog

All notable changes to this project will be documented in this file.

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
