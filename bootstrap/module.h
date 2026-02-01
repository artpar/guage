#ifndef MODULE_H
#define MODULE_H

#include "cell.h"
#include "strtable.h"
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
 * O(1) lookups via StrTable (SIMD SwissTable + SipHash).
 */

// Module load state for caching and circular detection
typedef enum {
    MODULE_UNLOADED,
    MODULE_LOADING,
    MODULE_LOADED
} ModuleLoadState;

// Module registry entry
typedef struct ModuleEntry {
    char* name;              // Module file path
    Cell* symbols;           // List of defined symbol names (as keywords)
    Cell* exports;           // List of exported symbol names (NULL = all exported)
    StrTable* export_set;    // O(1) export lookup (NULL = export all)
    StrTable* local_env;     // Module-local symbol→Cell* bindings (NULL until first define)
    Cell* dependencies;      // List of module names this module depends on
    char* version;           // Semantic version string (NULL if unset)
    time_t loaded_at;        // When loaded (Unix timestamp)
    size_t load_order;       // Load sequence (1, 2, 3...)
    ModuleLoadState load_state; // Load state for caching/circular detection
    Cell* cached_result;     // Cached evaluation result (when MODULE_LOADED)
} ModuleEntry;

// Global module registry
typedef struct {
    StrTable modules;        // module_name → ModuleEntry*
    StrTable symbol_index;   // symbol_name → module_name (char*)
    StrTable alias_table;    // alias → module_path (char*, owned strdup)
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
 */
ModuleEntry* module_registry_get_entry(const char* module_name);

/**
 * Add a dependency to a module
 * Records that module_name depends on dep_module_name
 */
void module_registry_add_dependency(const char* module_name, const char* dep_module_name);

/**
 * Get all dependencies of a module
 * Returns: List of module names (as strings) or nil if module not found
 */
Cell* module_registry_get_dependencies(const char* module_name);

/**
 * Set module version
 */
void module_registry_set_version(const char* module_name, const char* version);

/**
 * Get module version
 * Returns: Version string or NULL if not set
 */
const char* module_registry_get_version(const char* module_name);

/**
 * Set exported symbols for a module
 * Pass NULL to export all (default behavior)
 */
void module_registry_set_exports(const char* module_name, Cell* exports);

/**
 * Get exported symbols for a module
 * Returns: List of exported symbols, or all symbols if no exports set
 */
Cell* module_registry_get_exports(const char* module_name);

/**
 * Check if symbol is exported from module
 * Returns: true if exported (or if no export restrictions)
 */
int module_registry_is_exported(const char* module_name, const char* symbol);

/**
 * Detect circular dependencies starting from a module
 * Returns: List of cycle paths (list of lists), or nil if no cycles
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

/**
 * Store a symbol→value binding in a module's local environment
 */
void module_registry_define(const char* module_name, const char* symbol, Cell* value);

/**
 * Lookup a symbol in a module's local environment
 * Returns: Cell* value (retained) or NULL if not found
 */
Cell* module_registry_lookup(const char* module_name, const char* symbol);

/**
 * Register a module alias (e.g., "m" → "math.scm")
 */
void module_registry_set_alias(const char* alias, const char* module_path);

/**
 * Resolve a module alias to its path
 * Returns: Module path string or NULL if alias not found
 */
const char* module_registry_resolve_alias(const char* alias);

#endif // MODULE_H
