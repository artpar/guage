#ifndef MODULE_H
#define MODULE_H

#include "cell.h"
#include <time.h>

/**
 * Module Registry
 *
 * Transparent module system for first development:
 * - All modules queryable as first-class values
 * - Provenance tracking (know where symbols came from)
 * - No information hiding (everything visible for analysis)
 * - Module metadata accessible at runtime
 *
 * See docs/planning/AI_FIRST_MODULES.md for design philosophy.
 */

// Module registry entry
typedef struct ModuleEntry {
    char* name;              // Module file path
    Cell* symbols;           // List of defined symbol names (as keywords)
    Cell* exports;           // List of exported symbol names (NULL = all exported) - Day 70
    Cell* dependencies;      // List of module names this module depends on - Day 29
    char* version;           // Semantic version string (NULL if unset) - Day 70
    time_t loaded_at;        // When loaded (Unix timestamp)
    size_t load_order;       // Load sequence (1, 2, 3...) - Day 27
    struct ModuleEntry* next;// Linked list
} ModuleEntry;

// Global module registry
typedef struct {
    ModuleEntry* head;
    size_t count;
} ModuleRegistry;

/**
 * Initialize module registry (call at startup)
 */
void module_registry_init(void);

/**
 * Register a new module (called by ⋘ when loading file)
 * If module already registered, does nothing.
 */
void module_registry_add(const char* module_name);

/**
 * Add symbol to module's symbol list
 * Called by ≔ (define) during module loading
 */
void module_registry_add_symbol(const char* module_name, const char* symbol);

/**
 * List all loaded modules
 * Returns: List of module names as strings
 */
Cell* module_registry_list_modules(void);

/**
 * Find which module defines a symbol
 * Returns: Module name (string) or NULL if not found
 */
const char* module_registry_find_symbol(const char* symbol);

/**
 * Get all symbols defined by a module
 * Returns: List of symbols (as keywords) or nil if module not found
 */
Cell* module_registry_list_symbols(const char* module_name);

/**
 * Check if module is loaded
 * Returns: true if module exists in registry
 */
int module_registry_has_module(const char* module_name);

/**
 * Get module entry by name (for accessing metadata)
 * Returns: ModuleEntry pointer or NULL if not found
 * Day 27: Needed for enhanced provenance tracking
 */
ModuleEntry* module_registry_get_entry(const char* module_name);

/**
 * Add a dependency to a module
 * Records that module_name depends on dep_module_name
 * Day 29: Module dependency tracking
 */
void module_registry_add_dependency(const char* module_name, const char* dep_module_name);

/**
 * Get all dependencies of a module
 * Returns: List of module names (as strings) or nil if module not found
 * Day 29: Module dependency tracking
 */
Cell* module_registry_get_dependencies(const char* module_name);

/**
 * Set module version
 * Day 70: Module versioning support
 */
void module_registry_set_version(const char* module_name, const char* version);

/**
 * Get module version
 * Returns: Version string or NULL if not set
 * Day 70: Module versioning support
 */
const char* module_registry_get_version(const char* module_name);

/**
 * Set exported symbols for a module
 * Pass NULL to export all (default behavior)
 * Day 70: Selective exports
 */
void module_registry_set_exports(const char* module_name, Cell* exports);

/**
 * Get exported symbols for a module
 * Returns: List of exported symbols, or all symbols if no exports set
 * Day 70: Selective exports
 */
Cell* module_registry_get_exports(const char* module_name);

/**
 * Check if symbol is exported from module
 * Returns: true if exported (or if no export restrictions)
 * Day 70: Selective exports
 */
int module_registry_is_exported(const char* module_name, const char* symbol);

/**
 * Detect circular dependencies starting from a module
 * Returns: List of cycle paths (list of lists), or nil if no cycles
 * Day 70: Cycle detection
 */
Cell* module_registry_detect_cycles(const char* module_name);

/**
 * Free all module registry memory (call at shutdown)
 */
void module_registry_free(void);

/**
 * Set current loading module (for tracking symbol definitions)
 * Pass NULL to clear.
 */
void module_set_current_loading(const char* module_name);

/**
 * Get current loading module name
 * Returns: Module name or NULL if not loading
 */
const char* module_get_current_loading(void);

#endif // MODULE_H
