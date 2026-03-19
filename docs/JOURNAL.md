# Development Journal

## 2026-03-18 — Phase 1: Project Scaffolding

Started Geomys, a Gopher protocol browser for classic Macintosh Plus.

Adapted the project structure and build system from the sibling project [Flynn](https://github.com/ecliptik/flynn) (telnet/finger client). Flynn's infrastructure — MacTCP wrapper, DNS resolver, utility functions, build scripts, and Rez resource patterns — provided a solid foundation. Roughly 60% of the low-level infrastructure was copied verbatim; the rest was adapted for Gopher's simpler stateless request-response model.

Phase 1 deliverables:
- Directory structure: `src/`, `include/`, `resources/`, `scripts/`, `docs/`
- CMakeLists.txt with feature flags for compile-time configuration
- `scripts/build.sh` adapted from Flynn with Geomys presets
- Rez resources: menu bar (Apple/File/Edit/Favorites/Options), About dialog, SIZE, BNDL, FREF, ICN#
- `main.c` with Toolbox initialization and event loop
- `menus.c` with menu setup and dispatch
- `dialogs.c` with About Geomys dialog (version, build SHA, machine name)
- Utility modules from Flynn: `macutil.c`, `sysutil.c`, `MacTCP.h`
- Documentation: README, CHANGELOG, BUILD, JOURNAL, ROADMAP, About Geomys

Key decisions:
- **Creator code:** `GEOM`
- **Default home page:** `gopher://sdf.org` (user-changeable in Options)
- **C standard:** C89/C90 throughout (matches era, native to Retro68)
- **Memory budget:** ~270KB of 384KB partition, ~30% headroom
- **License:** ISC (same as Flynn)

Next: Phase 2 — Networking & Gopher Protocol Core (TCP, DNS, RFC 1436 parser).

## 2026-03-18 — Phase 2: Networking & Gopher Protocol Core

Added TCP/IP networking and the Gopher protocol engine. The MacTCP wrapper (`tcp.c`/`tcp.h`) and DNS resolver (`dns.c`/`dns.h`) were copied verbatim from Flynn. The connection state machine (`connection.c`) was adapted from Flynn's telnet connection logic — stripped out protocol/username fields and telnet-specific handling, added Gopher state tracking with default port 70.

Created the RFC 1436 protocol parser (`gopher.c`) which splits tab-delimited Gopher directory lines into GopherItem structs (type, display text, selector, host, port). Added a URI parser for RFC 4266 `gopher://` URLs. The type handler registry (`gopher_types.c`) maps all 18 item types (canonical: 0,1,2,3,4,5,6,7,8,9,g,I,T and non-canonical: d,h,i,p,r,s) to handler functions.

Tested successfully against `gopher://sdf.org` — DNS resolves, TCP connects on port 70, directory listing parses and displays with clickable items rendered in Monaco 9. The connection lifecycle (resolve → connect → send selector+CRLF → receive → parse → display) works end-to-end.

Next: Phase 3 — Browser Chrome & Window Layout (address bar, nav buttons, status bar).

## 2026-03-18 — Phase 3: Browser Chrome

Added the browser UI chrome that transforms the raw window into a proper browser. Navigation bar with Back/Forward/Refresh/Home buttons (20x20 bold QuickDraw icons), address bar (TextEdit field), and status bar at the bottom. Open URL dialog (Cmd-L) with pre-filled current URL. Replaced the modal "Resolving/Connecting" status window with non-blocking status bar updates. Added documentProc window with title bar, close box, and resizable grow box.

## 2026-03-18 — Phase 4: Scrollable Content

Added vertical scroll bar (standard Mac ControlHandle) with line/page/thumb scrolling via ControlActionUPP callback. Content clips to exclude the scrollbar column, fixing the spill issue from Phase 3. Per-row erase reduces flicker compared to full-area EraseRect. Grow box clipped to bottom-right corner and redrawn after scroll and status bar updates.

## 2026-03-18 — Phase 5: Search & Type Handlers

Implemented Type 7 search with query dialog — user clicks a search item, enters a query, results display as a directory listing. Fixed ModalDialog loop for EditText interaction (single call was dismissing on text field clicks). Added informational messages for all non-navigable types: download, image, telnet, RTF, sound, HTML external links. Error recovery now preserves previous page content when a connection fails — new buffers allocated before connection attempt, freed on failure.

## 2026-03-18 — Phase 6: Navigation History

10-entry history stack enables browser-style back/forward. All navigation routes through do_navigate_url for consistent history tracking. Back/Forward nav buttons and Cmd-[/Cmd-] keyboard shortcuts. Button states update after page load completes. Failed back/forward undoes the history position change so the user returns to the page they were on.

## 2026-03-18 — Phase 7: Settings, Favorites & Release

User-configurable home page via Options > Home Page dialog (with blank page option). Preferences persist to "Geomys Preferences" file (Preferences folder on System 7, volume root on System 6). Favorites system with Add (Cmd-D), Manage Favorites dialog (Add/Edit/Delete/Move Up/Move Down/Go To), and top 5 shown in Favorites menu. Page titles extracted from clicked item display names (cleaned of date/size metadata). Release script adapted from Flynn for Forgejo and GitHub. MVP feature-complete.

## 2026-03-18 — v0.2.1: Performance & Polish

Addressed all 8 remaining polish items from the v0.2.0 TODO list. Key architectural change: extracted `content_draw_row()` from the directory rendering loop, enabling targeted single-row redraws for both scroll and hover operations.

**Scroll performance:** Line scroll (arrow buttons) now uses `ScrollRect` to shift existing pixels, then draws only the 1-2 newly revealed rows. ~15x faster than full page redraw. Page scroll and thumb drag still use full redraw (acceptable since they replace the entire viewport).

**Hover performance:** Mouse hover now redraws only the 2 affected rows (old highlight removed, new highlight added) instead of the full page. ~12x faster. Early exit added when hover row hasn't changed.

**Page styles functional:** `content_draw_row()` includes style branching for all 3 modes. Traditional shows type labels (`TXT`, `DIR`), Plain shows underlined links without labels, Markdown shows bullet prefixes (`¥` in Mac Roman) for navigable items and `?` for search.

**CP437 integration:** Text page rendering now translates CP437 high bytes to Mac Roman equivalents. Fast path scans for bytes > 0x7F and skips translation for ASCII-clean lines (zero overhead). Glyph-type characters use `copy_char` fallback (single char substitution, no bitmap rendering — too slow on 68000).

**Gopher+ indicator:** Items with `has_plus` show `+` suffix on type labels in Traditional style (e.g., `TXT+`, `DIR+`).

**Prefs icon fix:** Found byte order bug in FREF resource — local icon ID was `$0100` instead of `$0001` (compared against Flynn's correct implementation). Desktop rebuild required after install.

**Grow box alignment:** Standardized clip rect in `handle_update()` to match `content_draw()` — both use `SCROLLBAR_WIDTH` with +1 on right/bottom edges.
