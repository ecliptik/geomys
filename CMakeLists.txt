cmake_minimum_required(VERSION 3.9)
project(Geomys VERSION 0.2.1)

# --- Feature flags with defaults (all ON for full preset) ---
option(GEOMYS_OFFSCREEN    "Enable offscreen double-buffer"     ON)
option(GEOMYS_STATUS_BAR   "Enable status bar"                  ON)
option(GEOMYS_FAVORITES    "Enable favorites system"            ON)
option(GEOMYS_COLOR        "Enable 256-color support"           OFF)
option(GEOMYS_DOWNLOAD     "Enable file downloads"              OFF)
option(GEOMYS_GOPHER_PLUS  "Enable Gopher+ protocol"           ON)
option(GEOMYS_GLYPHS       "Enable Unicode glyph rendering"    ON)
option(GEOMYS_CP437        "Enable CP437 character set"         ON)
option(GEOMYS_STYLES       "Enable page format styles"          ON)
option(GEOMYS_CACHE        "Enable local page caching"          ON)

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
    src/macutil.c
    src/sysutil.c
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
