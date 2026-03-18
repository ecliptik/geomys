cmake_minimum_required(VERSION 3.9)
project(Geomys VERSION 0.1.0)

# --- Feature flags with defaults ---
option(GEOMYS_OFFSCREEN    "Enable offscreen double-buffer"     OFF)
option(GEOMYS_STATUS_BAR   "Enable status bar"                  ON)
option(GEOMYS_FAVORITES    "Enable favorites system"            OFF)
option(GEOMYS_COLOR        "Enable 256-color support"           OFF)
option(GEOMYS_DOWNLOAD     "Enable file downloads"              OFF)

# --- Source files ---
set(GEOMYS_SOURCES
    src/main.c
    src/menus.c
    src/dialogs.c
    src/browser.c
    src/connection.c
    src/tcp.c
    src/dns.c
    src/gopher.c
    src/gopher_types.c
    src/macutil.c
    src/sysutil.c
)

# Conditional source files for future phases
#if(GEOMYS_FAVORITES)
#    list(APPEND GEOMYS_SOURCES src/favorites.c)
#endif()

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
