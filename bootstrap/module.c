#include "module.h"
#include "cell.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Global module registry
static ModuleRegistry registry = {NULL, 0};

// Current loading module (for tracking symbol definitions)
static const char* current_loading_module = NULL;

// Load order counter (increments with each module load) - Day 27
static size_t load_order_counter = 0;

void module_registry_init(void) {
    registry.head = NULL;
    registry.count = 0;
}

void module_registry_add(const char* module_name) {
    if (!module_name) return;

    // Check if already exists
    ModuleEntry* curr = registry.head;
    while (curr) {
        if (strcmp(curr->name, module_name) == 0) {
            return;  // Already registered
        }
        curr = curr->next;
    }

    // Create new entry
    ModuleEntry* entry = malloc(sizeof(ModuleEntry));
    if (!entry) {
        fprintf(stderr, "Error: Failed to allocate module entry\n");
        return;
    }

    entry->name = strdup(module_name);
    if (!entry->name) {
        fprintf(stderr, "Error: Failed to allocate module name\n");
        free(entry);
        return;
    }

    entry->symbols = cell_nil();
    cell_retain(entry->symbols);
    entry->exports = NULL;  // Day 70: NULL means export all
    entry->dependencies = cell_nil();  // Day 29: Initialize dependencies
    cell_retain(entry->dependencies);
    entry->version = NULL;  // Day 70: No version initially
    entry->loaded_at = time(NULL);
    entry->load_order = ++load_order_counter;  // Day 27: Track load sequence
    entry->next = registry.head;
    registry.head = entry;
    registry.count++;
}

void module_registry_add_symbol(const char* module_name, const char* symbol) {
    if (!module_name || !symbol) return;

    // Find module
    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            // Check if symbol already in list
            Cell* curr = entry->symbols;
            while (curr && !cell_is_nil(curr)) {
                Cell* sym = cell_car(curr);
                if (cell_is_symbol(sym) &&
                    strcmp(cell_get_symbol(sym), symbol) == 0) {
                    // Already in list
                    return;
                }
                curr = cell_cdr(curr);
            }

            // Add symbol to front of list
            Cell* new_sym = cell_symbol(symbol);
            Cell* new_list = cell_cons(new_sym, entry->symbols);
            cell_release(new_sym);
            cell_release(entry->symbols);
            entry->symbols = new_list;
            cell_retain(entry->symbols);
            return;
        }
        entry = entry->next;
    }

    // Module not found - this can happen if symbol defined before module registered
    // Just ignore it (module will be added later by ⋘)
}

Cell* module_registry_list_modules(void) {
    // Return list of module names (as strings, not symbols)
    Cell* result = cell_nil();

    // Build list in reverse order (most recent first)
    ModuleEntry* entry = registry.head;
    while (entry) {
        Cell* name_cell = cell_string(entry->name);
        Cell* new_result = cell_cons(name_cell, result);
        cell_release(name_cell);
        cell_release(result);
        result = new_result;

        entry = entry->next;
    }

    return result;
}

const char* module_registry_find_symbol(const char* symbol) {
    if (!symbol) return NULL;

    // Normalize symbol name - strip leading colon if present
    // Keywords like :square should match stored names like square
    const char* search_name = symbol;
    if (symbol[0] == ':') {
        search_name = symbol + 1;  // Skip the colon
    }

    // Search all modules for symbol
    // Return first match (most recently loaded)
    ModuleEntry* entry = registry.head;
    while (entry) {
        Cell* curr = entry->symbols;
        while (curr && !cell_is_nil(curr)) {
            Cell* sym = cell_car(curr);
            if (cell_is_symbol(sym)) {
                const char* stored_name = cell_get_symbol(sym);
                // Also normalize stored name if it has a colon
                if (stored_name[0] == ':') {
                    stored_name++;
                }
                if (strcmp(stored_name, search_name) == 0) {
                    return entry->name;
                }
            }
            curr = cell_cdr(curr);
        }
        entry = entry->next;
    }

    return NULL;  // Not found
}

Cell* module_registry_list_symbols(const char* module_name) {
    if (!module_name) return cell_nil();

    // Find module and return its symbols
    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            cell_retain(entry->symbols);
            return entry->symbols;
        }
        entry = entry->next;
    }

    // Module not found
    return cell_nil();
}

int module_registry_has_module(const char* module_name) {
    if (!module_name) return 0;

    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

ModuleEntry* module_registry_get_entry(const char* module_name) {
    if (!module_name) return NULL;

    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            return entry;  // Day 27: Return entry for metadata access
        }
        entry = entry->next;
    }
    return NULL;
}

void module_registry_add_dependency(const char* module_name, const char* dep_module_name) {
    if (!module_name || !dep_module_name) return;

    // Don't add self-dependencies
    if (strcmp(module_name, dep_module_name) == 0) return;

    // Find module
    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            // Check if dependency already in list
            Cell* curr = entry->dependencies;
            while (curr && !cell_is_nil(curr)) {
                Cell* dep = cell_car(curr);
                if (cell_is_string(dep) &&
                    strcmp(cell_get_string(dep), dep_module_name) == 0) {
                    // Already in list
                    return;
                }
                curr = cell_cdr(curr);
            }

            // Add dependency to front of list (as string)
            Cell* new_dep = cell_string(dep_module_name);
            Cell* new_list = cell_cons(new_dep, entry->dependencies);
            cell_release(new_dep);
            cell_release(entry->dependencies);
            entry->dependencies = new_list;
            cell_retain(entry->dependencies);
            return;
        }
        entry = entry->next;
    }

    // Module not found - ignore (module will be added later)
}

Cell* module_registry_get_dependencies(const char* module_name) {
    if (!module_name) return cell_nil();

    // Find module and return its dependencies
    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            cell_retain(entry->dependencies);
            return entry->dependencies;
        }
        entry = entry->next;
    }

    // Module not found
    return cell_nil();
}

void module_registry_free(void) {
    ModuleEntry* entry = registry.head;
    while (entry) {
        ModuleEntry* next = entry->next;
        free(entry->name);
        cell_release(entry->symbols);
        if (entry->exports) cell_release(entry->exports);  // Day 70
        cell_release(entry->dependencies);  // Day 29: Release dependencies
        if (entry->version) free(entry->version);  // Day 70
        free(entry);
        entry = next;
    }
    registry.head = NULL;
    registry.count = 0;
}

void module_set_current_loading(const char* module_name) {
    current_loading_module = module_name;
}

const char* module_get_current_loading(void) {
    return current_loading_module;
}

/* Day 70: Module versioning support */
void module_registry_set_version(const char* module_name, const char* version) {
    if (!module_name || !version) return;

    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            if (entry->version) free(entry->version);
            entry->version = strdup(version);
            return;
        }
        entry = entry->next;
    }
}

const char* module_registry_get_version(const char* module_name) {
    if (!module_name) return NULL;

    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            return entry->version;
        }
        entry = entry->next;
    }
    return NULL;
}

/* Day 70: Selective exports */
void module_registry_set_exports(const char* module_name, Cell* exports) {
    if (!module_name) return;

    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            if (entry->exports) cell_release(entry->exports);
            if (exports) {
                cell_retain(exports);
                entry->exports = exports;
            } else {
                entry->exports = NULL;
            }
            return;
        }
        entry = entry->next;
    }
}

Cell* module_registry_get_exports(const char* module_name) {
    if (!module_name) return cell_nil();

    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            if (entry->exports) {
                cell_retain(entry->exports);
                return entry->exports;
            } else {
                /* No explicit exports = export all symbols */
                cell_retain(entry->symbols);
                return entry->symbols;
            }
        }
        entry = entry->next;
    }
    return cell_nil();
}

int module_registry_is_exported(const char* module_name, const char* symbol) {
    if (!module_name || !symbol) return 0;

    ModuleEntry* entry = registry.head;
    while (entry) {
        if (strcmp(entry->name, module_name) == 0) {
            if (!entry->exports) {
                /* No explicit exports = everything exported */
                return 1;
            }

            /* Normalize symbol name */
            const char* search_name = symbol;
            if (symbol[0] == ':') search_name = symbol + 1;

            /* Check if in export list */
            Cell* curr = entry->exports;
            while (curr && !cell_is_nil(curr)) {
                Cell* exp = cell_car(curr);
                if (cell_is_symbol(exp)) {
                    const char* exp_name = cell_get_symbol(exp);
                    if (exp_name[0] == ':') exp_name++;
                    if (strcmp(exp_name, search_name) == 0) {
                        return 1;
                    }
                }
                curr = cell_cdr(curr);
            }
            return 0;  /* Not in export list */
        }
        entry = entry->next;
    }
    return 0;  /* Module not found */
}

/* Day 70: Cycle detection using DFS */
static int detect_cycles_dfs(const char* module, Cell** visited, Cell** rec_stack, Cell** cycles) {
    /* Add to visited */
    Cell* mod_cell = cell_string(module);
    Cell* new_visited = cell_cons(mod_cell, *visited);
    cell_release(mod_cell);
    cell_release(*visited);
    *visited = new_visited;

    /* Add to recursion stack */
    mod_cell = cell_string(module);
    Cell* new_stack = cell_cons(mod_cell, *rec_stack);
    cell_release(mod_cell);
    cell_release(*rec_stack);
    *rec_stack = new_stack;

    /* Get dependencies */
    Cell* deps = module_registry_get_dependencies(module);
    Cell* curr = deps;

    while (curr && !cell_is_nil(curr)) {
        Cell* dep = cell_car(curr);
        if (cell_is_string(dep)) {
            const char* dep_name = cell_get_string(dep);

            /* Check if in recursion stack (cycle!) */
            Cell* stack_check = *rec_stack;
            while (stack_check && !cell_is_nil(stack_check)) {
                Cell* stacked = cell_car(stack_check);
                if (cell_is_string(stacked) && strcmp(cell_get_string(stacked), dep_name) == 0) {
                    /* Found cycle - record it */
                    Cell* cycle = cell_nil();
                    Cell* s = *rec_stack;
                    while (s && !cell_is_nil(s)) {
                        Cell* m = cell_car(s);
                        Cell* new_cycle = cell_cons(m, cycle);
                        cell_release(cycle);
                        cycle = new_cycle;
                        s = cell_cdr(s);
                    }
                    Cell* new_cycles = cell_cons(cycle, *cycles);
                    cell_release(cycle);
                    cell_release(*cycles);
                    *cycles = new_cycles;
                    cell_release(deps);
                    return 1;
                }
                stack_check = cell_cdr(stack_check);
            }

            /* Check if not visited */
            int is_visited = 0;
            Cell* vis_check = *visited;
            while (vis_check && !cell_is_nil(vis_check)) {
                Cell* v = cell_car(vis_check);
                if (cell_is_string(v) && strcmp(cell_get_string(v), dep_name) == 0) {
                    is_visited = 1;
                    break;
                }
                vis_check = cell_cdr(vis_check);
            }

            if (!is_visited) {
                if (detect_cycles_dfs(dep_name, visited, rec_stack, cycles)) {
                    cell_release(deps);
                    return 1;
                }
            }
        }
        curr = cell_cdr(curr);
    }

    cell_release(deps);

    /* Remove from recursion stack */
    if (*rec_stack && !cell_is_nil(*rec_stack)) {
        Cell* old_stack = *rec_stack;
        *rec_stack = cell_cdr(*rec_stack);
        cell_retain(*rec_stack);
        cell_release(old_stack);
    }

    return 0;
}

Cell* module_registry_detect_cycles(const char* module_name) {
    if (!module_name) return cell_nil();

    Cell* visited = cell_nil();
    Cell* rec_stack = cell_nil();
    Cell* cycles = cell_nil();

    detect_cycles_dfs(module_name, &visited, &rec_stack, &cycles);

    cell_release(visited);
    cell_release(rec_stack);

    return cycles;
}
