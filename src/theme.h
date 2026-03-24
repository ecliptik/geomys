/*
 * theme.h - Theme engine for Geomys
 *
 * Provides named color themes for the Gopher browser.
 * Light and Dark themes work on monochrome systems.
 * Color themes require Color QuickDraw (Mac II+).
 */

#ifndef THEME_H
#define THEME_H

#ifdef GEOMYS_THEMES

/* Compact 3-byte RGB — expanded to RGBColor (16-bit) at runtime via x257 */
typedef struct {
	unsigned char r, g, b;
} ThemeRGB;

/* Theme color properties for a Gopher browser */
typedef struct {
	const char    *name;           /* menu display name */
	unsigned char  is_color;       /* 1 = requires Color QuickDraw */
	unsigned char  is_dark;        /* 1 = dark theme (for mono fallback) */

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

	/* Chrome stubs (for future chrome theming) */
	ThemeRGB chrome_bg;            /* nav bar / status bar background */
	ThemeRGB chrome_fg;            /* chrome text, icons */
	ThemeRGB addr_bg;              /* address bar background */
	ThemeRGB addr_fg;              /* address bar text */
} ThemeColors;

/* Theme indices — THEME_LIGHT must be 0 for backward compat */
#define THEME_LIGHT             0
#define THEME_DARK              1
#define THEME_SOLARIZED_LIGHT   2
#define THEME_SOLARIZED_DARK    3
#define THEME_TOKYO_LIGHT       4
#define THEME_TOKYO_DARK        5
#define THEME_GREEN_SCREEN      6
#define THEME_CLASSIC           7
#define THEME_PLATINUM          8

/* Total theme count (mono themes always available, color themes conditional) */
#define THEME_COUNT_MONO   2
#ifdef GEOMYS_COLOR
#define THEME_COUNT        9
#else
#define THEME_COUNT        2
#endif

/* Initialize theme system — call after color_detect() and prefs_load() */
void theme_init(short theme_id);

/* Get/set active theme */
const ThemeColors *theme_current(void);
void theme_set(short index);
short theme_get(void);

/* Get theme by index (for menu building) */
const ThemeColors *theme_get_by_index(short index);

/* Get count of usable themes (respects runtime color detection) */
short theme_usable_count(void);

/* Apply ThemeRGB to QuickDraw foreground/background (with cache) */
void theme_set_fg(const ThemeRGB *c);
void theme_set_bg(const ThemeRGB *c);

/* Invalidate color cache (call at start of each draw pass) */
void theme_reset_cache(void);

/* Is current theme dark? (for mono fallback rendering) */
short theme_is_dark(void);

/* Is current theme a color theme? */
short theme_is_color(void);

/* Restore port colors to black fg / white bg after themed drawing.
 * Must be called on every exit path that may have set theme colors. */
void theme_restore_colors(void);

#else /* !GEOMYS_THEMES */

#define THEME_LIGHT  0
#define THEME_COUNT  0
#define theme_init(id)          ((void)0)
#define theme_current()         ((void *)0)
#define theme_set(i)            ((void)0)
#define theme_get()             0
#define theme_get_by_index(i)   ((void *)0)
#define theme_usable_count()    0
#define theme_set_fg(c)         ((void)0)
#define theme_set_bg(c)         ((void)0)
#define theme_reset_cache()     ((void)0)
#define theme_is_dark()         0
#define theme_is_color()        0
#define theme_restore_colors()  ((void)0)

#endif /* GEOMYS_THEMES */

#endif /* THEME_H */
