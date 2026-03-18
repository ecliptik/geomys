# Geomys Roadmap

## Phase 1: Project Scaffolding & Build System
**Status: In Progress**

Directory structure, build system, and minimal running app — empty window with menu bar and About dialog.

- Directory structure and CMakeLists.txt with feature flags
- `scripts/build.sh` with preset support
- Rez resources: menus, About dialog, SIZE, BNDL, FREF, ICN#
- Toolbox initialization and event loop
- Menu bar: Apple, File, Edit, Favorites, Options
- About Geomys dialog with version and build SHA
- Utility modules adapted from Flynn
- Initial documentation

## Phase 2: Networking & Gopher Protocol Core

TCP connectivity and RFC 1436 Gopher protocol parser. Connect to a server, parse directory listings, display results.

- MacTCP wrapper and DNS resolver (from Flynn)
- Connection state machine: IDLE → RESOLVING → CONNECTING → SENDING → RECEIVING → DONE
- RFC 1436 line parser and GopherItem struct
- RFC 4266 URI parser (`gopher://host[:port]/type[selector]`)
- Type handler registry for all 18 item types
- Connection status window

## Phase 3: Browser Chrome & Window Layout

Full browser UI — address bar, navigation buttons, status bar, menus wired.

- Nav bar: Back/Forward/Refresh/Home buttons + address bar (TextEdit)
- Content area with scroll bar
- Status bar: connection state and page info
- Open URL dialog (Cmd-L)
- Keyboard/mouse event routing
- Full menu enable/disable based on state

## Phase 4: Content Display — Directories & Text

Scrollable content area with clickable directory listings and plain text files.

- Directory rendering: item type icons, clickable navigation, inverse highlight
- Text rendering: Monaco 9 monospaced, line offset index
- Scroll bar: page up/down, line up/down, thumb drag
- Efficient drawing: clipped to content area, visible items only

## Phase 5: Full Type Support & Search

All 18 Gopher types handled. Type 7 (search) fully functional.

- Type 7 search query dialog
- Type 2 CSO phone-book query
- Type h HTML URL display
- Download types (4,5,6,9,d): informational messages
- Image types (g,I,p): informational messages
- Telnet types (8,T): suggest Flynn
- Error type (3): inline error display

## Phase 6: Navigation History & Error Handling

Back/forward history, refresh, home page, address bar sync, robust error handling.

- 10-entry history stack (URL-only, re-fetch on navigate)
- Back/Forward/Refresh/Home button behavior
- Address bar and window title sync
- Cmd-[ / Cmd-] keyboard shortcuts
- Error handling: DNS failure, connection refused, timeout, partial content
- Cmd-. cancels in-progress connections

## Phase 7: Settings, Favorites, Double Buffering & Release

User preferences, favorites, flicker-free rendering, release packaging.

- Settings: home page, DNS server, font choice (persisted)
- Favorites: add/edit/delete, dynamic menu, Cmd-D
- Double buffering via offscreen BitMap (feature-flagged)
- Edit > Find (Cmd-F), File > Save Page As (Cmd-S)
- Build presets: minimal, lite, full
- Release scripts and documentation finalized

---

## Future Features (Post-MVP)

- **256-color support** — Color QuickDraw on System 7, automatic detection
- **File downloads** — Save binary files (types 4, 5, 6, 9, d) to disk
- **Image display** — Inline GIF/PNG viewing (types g, I, p)
- **Multi-window browsing** — Multiple simultaneous Gopher sessions
- **System 7 enhancements** — Preferences folder, Apple Events, Notification Manager
- **Themes** — Classic, Platinum, Solarized, Tokyo Night, Green Screen
- **Keyboard navigation** — Tab through links, Enter to follow
- **Gopher+ awareness** — Detect and handle `+` modifiers gracefully
