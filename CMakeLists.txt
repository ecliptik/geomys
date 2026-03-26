cmake_minimum_required(VERSION 3.9)
project(Geomys VERSION 0.15.1)

# --- Feature flags with defaults (all ON for full preset) ---
option(GEOMYS_OFFSCREEN    "Enable offscreen double-buffer"     ON)
option(GEOMYS_STATUS_BAR   "Enable status bar"                  ON)
option(GEOMYS_FAVORITES    "Enable favorites system"            ON)
option(GEOMYS_COLOR        "Enable 256-color support"           OFF)
option(GEOMYS_THEMES       "Enable theme system"                ON)
option(GEOMYS_DOWNLOAD     "Enable file downloads"              ON)
option(GEOMYS_GOPHER_PLUS  "Enable Gopher+ protocol"           ON)
option(GEOMYS_GLYPHS       "Enable Unicode glyph rendering"    ON)
option(GEOMYS_CP437        "Enable CP437 character set"         ON)
option(GEOMYS_STYLES       "Enable page format styles"          ON)
option(GEOMYS_CACHE        "Enable local page caching"          ON)
option(GEOMYS_CLIPBOARD    "Enable clipboard and text selection" ON)
option(GEOMYS_PRINT        "Enable printing support"            ON)
option(GEOMYS_HTML         "Enable HTML tag stripping"          ON)
option(GEOMYS_TELNET       "Enable telnet connection info"      ON)
option(GEOMYS_DRAG         "Enable Drag Manager support"        ON)
option(GEOMYS_APPLESCRIPT  "Enable AppleScript support"         ON)

# Maximum simultaneous windows (1 = single-window, zero overhead)
if(NOT DEFINED GEOMYS_MAX_WINDOWS)
    set(GEOMYS_MAX_WINDOWS 1)
endif()

# Scale cache slots based on window count (more windows = more cache needed)
if(NOT DEFINED CACHE_MAX)
    if(GEOMYS_MAX_WINDOWS EQUAL 1)
        set(CACHE_MAX 3)
    elseif(GEOMYS_MAX_WINDOWS EQUAL 2)
        set(CACHE_MAX 4)
    else()
        set(CACHE_MAX 6)
    endif()
endif()

# --- Core source files (always compiled) ---
set(GEOMYS_SOURCES
    src/main.c
    src/menus.c
    src/dialogs.c
    src/browser.c
    src/content.c
    src/history.c
    src/settings.c
    src/connection.c
    src/tcp.c
    src/dns.c
    src/gopher.c
    src/gopher_types.c
    src/gopher_icons.c
    src/macutil.c
    src/sysutil.c
    src/session.c
)

# --- Conditional source files ---
if(GEOMYS_FAVORITES)
    list(APPEND GEOMYS_SOURCES src/favorites.c)
endif()
if(GEOMYS_OFFSCREEN)
    list(APPEND GEOMYS_SOURCES src/offscreen.c)
endif()
if(GEOMYS_GLYPHS)
    list(APPEND GEOMYS_SOURCES src/glyphs.c)
endif()
if(GEOMYS_CP437)
    list(APPEND GEOMYS_SOURCES src/cp437.c)
endif()
if(GEOMYS_CACHE)
    list(APPEND GEOMYS_SOURCES src/cache.c)
endif()
if(GEOMYS_GOPHER_PLUS)
    list(APPEND GEOMYS_SOURCES src/gopherplus.c)
endif()
if(GEOMYS_CLIPBOARD)
    list(APPEND GEOMYS_SOURCES src/clipboard.c)
endif()
if(GEOMYS_DOWNLOAD)
    list(APPEND GEOMYS_SOURCES src/savefile.c src/imgparse.c)
endif()
if(GEOMYS_PRINT)
    list(APPEND GEOMYS_SOURCES src/print.c)
endif()
if(GEOMYS_HTML)
    list(APPEND GEOMYS_SOURCES src/html.c)
endif()
if(GEOMYS_COLOR)
    list(APPEND GEOMYS_SOURCES src/color.c)
endif()
if(GEOMYS_THEMES)
    list(APPEND GEOMYS_SOURCES src/theme.c)
endif()
if(GEOMYS_DRAG)
    list(APPEND GEOMYS_SOURCES src/drag.c)
endif()

add_application(Geomys ${GEOMYS_SOURCES} resources/geomys.r)

# Include path for MacTCP.h and local headers
target_include_directories(Geomys PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- Pass all flags to C code as compile definitions ---
target_compile_definitions(Geomys PRIVATE
    $<$<BOOL:${GEOMYS_OFFSCREEN}>:GEOMYS_OFFSCREEN>
    $<$<BOOL:${GEOMYS_STATUS_BAR}>:GEOMYS_STATUS_BAR>
    $<$<BOOL:${GEOMYS_FAVORITES}>:GEOMYS_FAVORITES>
    $<$<BOOL:${GEOMYS_COLOR}>:GEOMYS_COLOR>
    $<$<BOOL:${GEOMYS_DOWNLOAD}>:GEOMYS_DOWNLOAD>
    $<$<BOOL:${GEOMYS_GOPHER_PLUS}>:GEOMYS_GOPHER_PLUS>
    $<$<BOOL:${GEOMYS_GLYPHS}>:GEOMYS_GLYPHS>
    $<$<BOOL:${GEOMYS_CP437}>:GEOMYS_CP437>
    $<$<BOOL:${GEOMYS_STYLES}>:GEOMYS_STYLES>
    $<$<BOOL:${GEOMYS_CACHE}>:GEOMYS_CACHE>
    $<$<BOOL:${GEOMYS_CLIPBOARD}>:GEOMYS_CLIPBOARD>
    $<$<BOOL:${GEOMYS_THEMES}>:GEOMYS_THEMES>
    $<$<BOOL:${GEOMYS_PRINT}>:GEOMYS_PRINT>
    $<$<BOOL:${GEOMYS_HTML}>:GEOMYS_HTML>
    $<$<BOOL:${GEOMYS_TELNET}>:GEOMYS_TELNET>
    $<$<BOOL:${GEOMYS_DRAG}>:GEOMYS_DRAG>
    $<$<BOOL:${GEOMYS_APPLESCRIPT}>:GEOMYS_APPLESCRIPT>
    GEOMYS_MAX_WINDOWS=${GEOMYS_MAX_WINDOWS}
    CACHE_MAX=${CACHE_MAX}
)

# Target 68000 CPU (Mac Plus compatible) and optimize for size
target_compile_options(Geomys PRIVATE
    -m68000
    -Os
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
)

# Link-time dead code/data elimination via --gc-sections
set_target_properties(Geomys PROPERTIES
    LINK_FLAGS "-Wl,-gc-sections"
)
