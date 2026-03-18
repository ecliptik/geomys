# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

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
