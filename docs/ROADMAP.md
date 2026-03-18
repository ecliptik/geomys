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

## Future Features (Post-MVP)

- **256-color support** — Color QuickDraw on System 7, automatic detection
- **File downloads** — Save binary files (types 4, 5, 6, 9, d) to disk
- **Image display** — Inline GIF/PNG viewing (types g, I, p)
- **Multi-window browsing** — Multiple simultaneous Gopher sessions
- **System 7 enhancements** — Preferences folder, Apple Events, Notification Manager
- **Themes** — Classic, Platinum, Solarized, Tokyo Night, Green Screen
- **Keyboard navigation** — Tab through links, Enter to follow
