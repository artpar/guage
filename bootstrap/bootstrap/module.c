/* module.c - AI-first module registry implementation */

#include "module.h"
#include "cell.h"
#include <stdlib.h>
#include <string.h>

/* Global module registry */
static ModuleInfo* modules = NULL;
static int module_count = 0;
static int module_capacity = 0;

/* Initialize module registry */
void module_init(void) {
    module_capacity = 16;
    modules = (ModuleInfo*)malloc(sizeof(ModuleInfo) * module_capacity);
    module_count = 0;
}

/* Register a loaded module */
void module_register(const char* path, Cell* definitions) {
    /* Check if module already registered (reload case) */
    for (int i = 0; i < module_count; i++) {
        if (strcmp(modules[i].path, path) == 0) {
            /* Update existing entry */
            cell_release(modules[i].definitions);
            cell_retain(definitions);
            modules[i].definitions = definitions;
            return;
        }
    }

    /* Expand capacity if needed */
    if (module_count >= module_capacity) {
        module_capacity *= 2;
        modules = (ModuleInfo*)realloc(modules, sizeof(ModuleInfo) * module_capacity);
    }

    /* Add new module */
    modules[module_count].path = strdup(path);
    cell_retain(definitions);
    modules[module_count].definitions = definitions;
    modules[module_count].exports = NULL;
    modules[module_count].dependencies = NULL;
    module_count++;
}

/* Query all loaded modules - returns list of paths */
Cell* module_list_all(void) {
    Cell* result = cell_nil();

    /* Build list in reverse order (will reverse at end) */
    for (int i = module_count - 1; i >= 0; i--) {
        Cell* path_str = cell_string(modules[i].path);
        result = cell_cons(path_str, result);
    }

    return result;
}

/* Get module metadata by path */
Cell* module_get(const char* path) {
    /* Find module */
    for (int i = 0; i < module_count; i++) {
        if (strcmp(modules[i].path, path) == 0) {
            /* Build Module structure */
            /* Structure: ⊙ :Module :path :defs :exports :deps */

            /* Build fields list */
            Cell* path_field = cell_string(modules[i].path);

            Cell* defs_field = modules[i].definitions;
            cell_retain(defs_field);

            Cell* exports_field = modules[i].exports ? modules[i].exports : cell_nil();
            cell_retain(exports_field);

            Cell* deps_field = modules[i].dependencies ? modules[i].dependencies : cell_nil();
            cell_retain(deps_field);

            /* Create structure using ⊙ pattern */
            /* For now, return a simple structure with fields */
            /* Type tag */
            Cell* type_tag = cell_symbol("Module");

            /* Build field list: (path defs exports deps) */
            Cell* field_list = cell_nil();
            field_list = cell_cons(deps_field, field_list);
            field_list = cell_cons(exports_field, field_list);
            field_list = cell_cons(defs_field, field_list);
            field_list = cell_cons(path_field, field_list);

            /* Create structure: (Module . (path defs exports deps)) */
            Cell* structure = cell_cons(type_tag, field_list);

            /* Mark as structure (using existing cell_struct infrastructure if available) */
            /* For now, just return the cons cell structure */
            return structure;
        }
    }

    /* Not found */
    return cell_error(":module-not-found", cell_string(path));
}

/* Find which module defined a symbol */
const char* module_find_symbol(const char* symbol) {
    /* Search through all modules */
    for (int i = 0; i < module_count; i++) {
        /* Check if symbol in module's definitions */
        Cell* defs = modules[i].definitions;

        /* Walk through definitions list */
        while (defs && cell_is_pair(defs)) {
            Cell* def = cell_car(defs);

            /* Each def is (symbol . value) */
            if (cell_is_pair(def)) {
                Cell* sym = cell_car(def);
                if (cell_is_symbol(sym)) {
                    const char* sym_name = cell_get_symbol(sym);
                    if (strcmp(sym_name, symbol) == 0) {
                        return modules[i].path;
                    }
                }
            }

            defs = cell_cdr(defs);
        }
    }

    return NULL;  /* Not found */
}

/* Set exports for module */
void module_set_exports(const char* path, Cell* exports) {
    for (int i = 0; i < module_count; i++) {
        if (strcmp(modules[i].path, path) == 0) {
            if (modules[i].exports) {
                cell_release(modules[i].exports);
            }
            cell_retain(exports);
            modules[i].exports = exports;
            return;
        }
    }
}

/* Set dependencies for module */
void module_set_dependencies(const char* path, Cell* dependencies) {
    for (int i = 0; i < module_count; i++) {
        if (strcmp(modules[i].path, path) == 0) {
            if (modules[i].dependencies) {
                cell_release(modules[i].dependencies);
            }
            cell_retain(dependencies);
            modules[i].dependencies = dependencies;
            return;
        }
    }
}

/* Cleanup module registry */
void module_cleanup(void) {
    for (int i = 0; i < module_count; i++) {
        free(modules[i].path);
        cell_release(modules[i].definitions);
        if (modules[i].exports) {
            cell_release(modules[i].exports);
        }
        if (modules[i].dependencies) {
            cell_release(modules[i].dependencies);
        }
    }
    free(modules);
    modules = NULL;
    module_count = 0;
    module_capacity = 0;
}
