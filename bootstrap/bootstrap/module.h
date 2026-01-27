/* module.h - AI-first module registry system
 *
 * Tracks loaded modules with full metadata for AI queries.
 * Maintains backwards compatibility - ⋘ still works the same way.
 *
 * Key principles:
 * - Everything queryable (no information hiding)
 * - Provenance tracking (where did symbols come from?)
 * - Transparent by design (AI can see all code)
 */

#ifndef MODULE_H
#define MODULE_H

#include "cell.h"

/* Module metadata structure */
typedef struct {
    char* path;              /* File path */
    Cell* definitions;       /* List of (symbol . value) pairs */
    Cell* exports;           /* List of exported symbols (or NULL for all) */
    Cell* dependencies;      /* List of dependency paths */
} ModuleInfo;

/* Initialize module registry */
void module_init(void);

/* Register a loaded module */
void module_register(const char* path, Cell* definitions);

/* Query all loaded modules */
/* Returns list of paths */
Cell* module_list_all(void);

/* Get module metadata by path */
/* Returns Module structure or error */
Cell* module_get(const char* path);

/* Find which module defined a symbol */
/* Returns path string or NULL */
const char* module_find_symbol(const char* symbol);

/* Set exports for module (called by ⊙◇ primitive) */
void module_set_exports(const char* path, Cell* exports);

/* Set dependencies for module */
void module_set_dependencies(const char* path, Cell* dependencies);

/* Cleanup module registry */
void module_cleanup(void);

#endif /* MODULE_H */
