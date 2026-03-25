# Geomys Theme Guide

This guide covers the Geomys theme system: how themes work, how to create custom themes, and how to integrate them into the build.

## Overview

Geomys uses a compile-time theme system. Each theme is a static `ThemeColors` struct defined in a header file under `src/themes/`. Themes are compiled into the binary — there is no runtime theme loading. This keeps memory usage minimal and avoids heap allocation for theme data.

Two categories of themes exist:

- **Mono themes** (Light, Dark) — work on all systems, including the Mac Plus. These use only black and white values; color fields are present but ignored on monochrome hardware.
- **Color themes** (Solarized, Tokyo Night, Green Screen, Classic, Platinum) — require a Mac II or later with Color QuickDraw. Detected at runtime via `SysEnvirons()`. Color themes are only compiled when `GEOMYS_COLOR=ON`.

## Built-in Themes

| Theme | Type | Description |
|-------|------|-------------|
| Light | Mono | White background, black text. Default theme, works everywhere. |
| Dark | Mono | Black background, white text. Uses `srcBic` rendering for flicker-free display. |
| Solarized Light | Color | Ethan Schoonover's Solarized palette, light variant. |
| Solarized Dark | Color | Solarized palette, dark variant. |
| Tokyo Night Light | Color | Based on the Tokyo Night color scheme, light variant. |
| Tokyo Night Dark | Color | Tokyo Night, dark variant. |
| Green Screen | Color | Phosphor green on black, classic CRT terminal aesthetic. |
| Classic | Color | 1990s web browser colors — blue links, purple visited, gray background. |
| Platinum | Color | Mac OS 8/9 Appearance Manager inspired — gray background, purple accents. |

## Theme Architecture

### Color Representation

Colors use a compact 3-byte `ThemeRGB` struct:

```c
typedef struct {
    unsigned char r, g, b;
} ThemeRGB;
```

At runtime, values are expanded to 16-bit `RGBColor` for QuickDraw via `x * 257` multiplication (maps 0x00-0xFF to 0x0000-0xFFFF). For best results on 256-color systems, choose colors from the 6x6x6 RGB cube (values: 0x00, 0x33, 0x66, 0x99, 0xCC, 0xFF).

### ThemeColors Struct

Every theme defines a `ThemeColors` struct with these fields:

```c
typedef struct {
    const char    *name;           /* menu display name */
    unsigned char  is_color;       /* 1 = requires Color QuickDraw */
    unsigned char  is_dark;        /* 1 = dark theme */

    /* Content area */
    ThemeRGB bg;                   /* content background */
    ThemeRGB text;                 /* info items / plain text */
    ThemeRGB link;                 /* navigable links (dirs, text files) */
    ThemeRGB link_search;          /* search items (type 7) */
    ThemeRGB link_external;        /* external/unsupported items */
    ThemeRGB link_download;        /* download items (files, images) */
    ThemeRGB link_error;           /* error items (type 3) */
    ThemeRGB label;                /* type labels (DIR, TXT, etc.) */
    ThemeRGB metadata;             /* right-aligned metadata (host:port) */
    ThemeRGB hover_bg;             /* hover row highlight background */

    /* Selection */
    ThemeRGB sel_bg;               /* selection highlight background */
    ThemeRGB sel_fg;               /* selected text foreground */
} ThemeColors;
```

#### Field Reference

**Metadata:**
| Field | Purpose |
|-------|---------|
| `name` | Displayed in the Options > Theme menu. Keep it short. |
| `is_color` | Set to `1` for themes that need Color QuickDraw. Set to `0` for mono-safe themes. |
| `is_dark` | Set to `1` for dark backgrounds. Controls mono fallback rendering mode (`srcBic`). |

**Content area** — these colors appear in the page content region:
| Field | Used for |
|-------|----------|
| `bg` | Background fill for the content area |
| `text` | Informational text (type `i`) and plain text pages |
| `link` | Navigable items: directories (type `1`), text files (type `0`) |
| `link_search` | Search/index items (type `7`) |
| `link_external` | External links: telnet (type `8`/`T`), HTML (type `h`) |
| `link_download` | Downloadable items: binaries (type `9`), images (type `g`/`I`/`p`), archives |
| `link_error` | Error items (type `3`) |
| `label` | Type labels (e.g., `DIR`, `TXT`, `BIN`) |
| `metadata` | Right-aligned host:port metadata |
| `hover_bg` | Background highlight when the cursor hovers over a row |

**Selection:**
| Field | Used for |
|-------|----------|
| `sel_bg` | Background color for text selection highlight |
| `sel_fg` | Foreground color for selected text |

## Creating a Custom Theme

### Step 1: Create the Header File

Create a new file in `src/themes/`. Name it after your theme using snake_case:

```
src/themes/my_theme.h
```

Use an existing theme as a template. Here is a minimal example:

```c
/*
 * themes/my_theme.h - My Custom Theme
 * Brief description of the theme's aesthetic.
 * Requires Color QuickDraw.
 */

static const ThemeColors theme_my_theme = {
    "My Theme",  /* name */
    1,           /* is_color (1 = color, 0 = mono) */
    0,           /* is_dark  (1 = dark,  0 = light) */

    /* Content area */
    { 0xFA, 0xFA, 0xFA },  /* bg */
    { 0x33, 0x33, 0x33 },  /* text */
    { 0x00, 0x66, 0xCC },  /* link */
    { 0x99, 0x66, 0x00 },  /* link_search */
    { 0x66, 0x66, 0x99 },  /* link_external */
    { 0x00, 0x99, 0x66 },  /* link_download */
    { 0xCC, 0x33, 0x00 },  /* link_error */
    { 0x66, 0x66, 0x66 },  /* label */
    { 0x99, 0x99, 0x99 },  /* metadata */
    { 0xEE, 0xEE, 0xEE },  /* hover_bg */

    /* Selection */
    { 0x00, 0x66, 0xCC },  /* sel_bg */
    { 0xFF, 0xFF, 0xFF },  /* sel_fg */
};
```

### Step 2: Register the Theme

Three files need changes to register a new theme.

**`src/theme.h`** — add a theme index constant and update the count:

```c
/* Add after the last existing theme index */
#define THEME_MY_THEME          9

/* Update THEME_COUNT (inside the #ifdef GEOMYS_COLOR block) */
#define THEME_COUNT        10    /* was 9 */
```

Theme indices must be sequential starting from 0. Mono themes occupy indices 0-1; color themes follow.

**`src/theme.c`** — include the header and add to the theme table:

```c
/* Add with the other color theme includes */
#ifdef GEOMYS_COLOR
#include "themes/my_theme.h"
#endif

/* Add to theme_table[] */
static const ThemeColors *theme_table[] = {
    &theme_light,
    &theme_dark,
#ifdef GEOMYS_COLOR
    &theme_solarized_light,
    /* ... existing themes ... */
    &theme_platinum,
    &theme_my_theme,        /* new */
#endif
};
```

The order in `theme_table[]` must match the `#define` indices.

**`resources/geomys.r`** — add the menu item to the Theme menu (MENU 137):

```rez
resource 'MENU' (137, "Theme") {
    137, textMenuProc, allEnabled, enabled, "Theme",
    {
        /* ... existing items ... */
        "Platinum", noIcon, noKey, noMark, plain;
        "My Theme", noIcon, noKey, noMark, plain   /* new */
    }
};
```

### Step 3: Update Menu Constants

In `src/main.h`, add a menu item constant for the new theme:

```c
#define THEME_ITEM_MY_THEME        11
#define THEME_ITEM_LAST            11    /* was 10 */
```

The menu item numbers account for the separator between mono and color themes (item 3), so color theme items start at 4.

### Step 4: Build and Test

Build with the `full` preset to include color support:

```bash
./scripts/build.sh --preset full
```

The theme will appear in Options > Theme. On monochrome systems, color themes are automatically grayed out in the menu.

## Design Guidelines

### Color Contrast

Ensure sufficient contrast between text/link colors and the background. Gopher content is text-heavy — readability is the primary concern. Test with both directory listings and long text pages.

### Dark Themes

Set `is_dark = 1` for dark-background themes. This flag controls:
- **Mono fallback**: On monochrome systems, dark themes use `srcBic` transfer mode (white-on-black) instead of `srcOr` (black-on-white).
- **Hover visibility**: The hover background must be visibly distinct from the content background on dark backgrounds. Use a slightly lighter shade.

### Hover and Selection

- `hover_bg` should be subtle — a slight shift from `bg`, not a bold color. Users hover constantly while browsing.
- `sel_bg` and `sel_fg` should have high contrast with each other. The selection is temporary but must be clearly visible.

### Link Differentiation

Gopher has distinct item types that benefit from color differentiation:
- **`link`** (directories, text) — the most common navigable type; make it prominent.
- **`link_search`** — search items should stand out since they trigger a query dialog.
- **`link_error`** — use a warm/red tone to signal errors visually.
- **`link_download`** — downloads and images; a distinct color helps users identify binary content.
- **`link_external`** — telnet/HTML items that leave the Gopher browser; consider a muted or secondary color.

### 256-Color Palette

For maximum compatibility on 256-color Macs, stick to the 6x6x6 RGB cube. The safe values for each channel are:

```
0x00  0x33  0x66  0x99  0xCC  0xFF
```

Colors outside this cube may dither on 8-bit displays. The built-in themes all use cube-safe values.

### Mono Theme Constraints

If creating a mono theme (`is_color = 0`):
- All color fields must be either `{ 0x00, 0x00, 0x00 }` (black) or `{ 0xFF, 0xFF, 0xFF }` (white).
- The theme will be available on all systems including the Mac Plus.
- Mono themes are always compiled regardless of `GEOMYS_COLOR`.

## Build System

Themes are controlled by two feature flags:

| Flag | Default | Effect |
|------|---------|--------|
| `GEOMYS_THEMES` | ON | Compiles the theme engine (`theme.c`). When OFF, all theme API calls become no-ops. |
| `GEOMYS_COLOR` | OFF | Compiles Color QuickDraw support (`color.c`) and all color themes. When OFF, only Light and Dark are available. |

Build presets:

| Preset | `GEOMYS_THEMES` | `GEOMYS_COLOR` | Available Themes |
|--------|-----------------|----------------|------------------|
| `full` | ON | ON | All 9 (mono + color) |
| `lite` | ON | OFF | Light, Dark only |
| `minimal` | OFF | OFF | None (no theme engine) |

Override with CLI flags:

```bash
./scripts/build.sh --preset lite --color     # lite features + color themes
./scripts/build.sh --no-themes               # disable theme engine entirely
```

## File Reference

```
src/
  theme.h             # ThemeColors struct, theme API, index constants
  theme.c             # Theme engine: init, set, get, color caching
  color.h             # Color QuickDraw detection (g_has_color_qd)
  color.c             # Runtime SysEnvirons() detection
  themes/
    light.h           # Light (mono, default)
    dark.h            # Dark (mono)
    solarized_light.h # Solarized Light (color)
    solarized_dark.h  # Solarized Dark (color)
    tokyo_light.h     # Tokyo Night Light (color)
    tokyo_dark.h      # Tokyo Night Dark (color)
    green_screen.h    # Green Screen (color)
    classic.h         # Classic (color)
    platinum.h        # Platinum (color)
resources/
  geomys.r            # Theme menu (MENU 137)
```
