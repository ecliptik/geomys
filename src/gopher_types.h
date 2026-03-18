/*
 * gopher_types.h - Gopher item type registry
 */

#ifndef GOPHER_TYPES_H
#define GOPHER_TYPES_H

typedef struct {
	char        type;
	const char  *label;
	Boolean     navigable;  /* true if clicking should navigate */
} GopherTypeInfo;

/* Look up type info for a Gopher item type character.
 * Returns pointer to static info struct, or default for unknown types. */
const GopherTypeInfo *gopher_type_info(char type);

/* Return a short label for display (e.g., "TXT", "DIR", "?") */
const char *gopher_type_label(char type);

/* Return true if this type is navigable (click should fetch) */
Boolean gopher_type_navigable(char type);

#endif /* GOPHER_TYPES_H */
