#include "module.h"
#include "cell.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* Global module registry — StrTable-backed O(1) lookups */
static ModuleRegistry registry;

/* Current loading module (for tracking symbol definitions) */
static const char* current_loading_module = NULL;

/* Load order counter (increments with each module load) */
static size_t load_order_counter = 0;

/* Free a ModuleEntry (callback for strtable_free) */
static void free_module_entry(void* p) {
    ModuleEntry* entry = (ModuleEntry*)p;
    free(entry->name);
    cell_release(entry->symbols);
    if (entry->exports) cell_release(entry->exports);
    if (entry->export_set) {
        strtable_free(entry->export_set, NULL);
        free(entry->export_set);
    }
    cell_release(entry->dependencies);
    if (entry->version) free(entry->version);
    if (entry->cached_result) cell_release(entry->cached_result);
    free(entry);
}

void module_registry_init(void) {
    strtable_init(&registry.modules, 64);
    strtable_init(&registry.symbol_index, 256);
    registry.count = 0;
}

void module_registry_add(const char* module_name) {
    if (!module_name) return;

    /* O(1) check if already exists */
    if (strtable_get(&registry.modules, module_name)) return;

    /* Create new entry */
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
    entry->exports = NULL;
    entry->export_set = NULL;
    entry->dependencies = cell_nil();
    cell_retain(entry->dependencies);
    entry->version = NULL;
    entry->loaded_at = time(NULL);
    entry->load_order = ++load_order_counter;
    entry->load_state = MODULE_UNLOADED;
    entry->cached_result = NULL;

    strtable_put(&registry.modules, module_name, entry);
    registry.count++;
}

void module_registry_add_symbol(const char* module_name, const char* symbol) {
    if (!module_name || !symbol) return;

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return;

    /* Normalize symbol name — strip leading colon */
    const char* norm = symbol;
    if (symbol[0] == ':') norm = symbol + 1;

    /* Check if symbol already in list (still Cell list for ordered iteration) */
    Cell* curr = entry->symbols;
    while (curr && !cell_is_nil(curr)) {
        Cell* sym = cell_car(curr);
        if (cell_is_symbol(sym)) {
            const char* stored = cell_get_symbol(sym);
            if (stored[0] == ':') stored++;
            if (strcmp(stored, norm) == 0) return;
        }
        curr = cell_cdr(curr);
    }

    /* Add symbol to front of list */
    Cell* new_sym = cell_symbol(symbol);
    Cell* new_list = cell_cons(new_sym, entry->symbols);
    cell_release(new_sym);
    cell_release(entry->symbols);
    entry->symbols = new_list;
    cell_retain(entry->symbols);

    /* Update global symbol→module index (O(1) lookup) */
    strtable_put(&registry.symbol_index, norm, entry->name);
}

Cell* module_registry_list_modules(void) {
    Cell* result = cell_nil();

    /* Iterate all slots in the StrTable */
    for (uint32_t i = 0; i < registry.modules.cap; i++) {
        if (registry.modules.ctrl[i] != CTRL_EMPTY &&
            registry.modules.ctrl[i] != CTRL_DELETED) {
            ModuleEntry* entry = (ModuleEntry*)registry.modules.slots[i].value;
            Cell* name_cell = cell_string(entry->name);
            Cell* new_result = cell_cons(name_cell, result);
            cell_release(name_cell);
            cell_release(result);
            result = new_result;
        }
    }

    return result;
}

const char* module_registry_find_symbol(const char* symbol) {
    if (!symbol) return NULL;

    /* Normalize — strip leading colon */
    const char* search_name = symbol;
    if (symbol[0] == ':') search_name = symbol + 1;

    /* O(1) lookup via symbol_index */
    return (const char*)strtable_get(&registry.symbol_index, search_name);
}

Cell* module_registry_list_symbols(const char* module_name) {
    if (!module_name) return cell_nil();

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return cell_nil();

    cell_retain(entry->symbols);
    return entry->symbols;
}

int module_registry_has_module(const char* module_name) {
    if (!module_name) return 0;
    return strtable_get(&registry.modules, module_name) != NULL;
}

ModuleEntry* module_registry_get_entry(const char* module_name) {
    if (!module_name) return NULL;
    return (ModuleEntry*)strtable_get(&registry.modules, module_name);
}

void module_registry_add_dependency(const char* module_name, const char* dep_module_name) {
    if (!module_name || !dep_module_name) return;
    if (strcmp(module_name, dep_module_name) == 0) return;

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return;

    /* Check if dependency already in list */
    Cell* curr = entry->dependencies;
    while (curr && !cell_is_nil(curr)) {
        Cell* dep = cell_car(curr);
        if (cell_is_string(dep) &&
            strcmp(cell_get_string(dep), dep_module_name) == 0) {
            return;
        }
        curr = cell_cdr(curr);
    }

    /* Add dependency to front of list */
    Cell* new_dep = cell_string(dep_module_name);
    Cell* new_list = cell_cons(new_dep, entry->dependencies);
    cell_release(new_dep);
    cell_release(entry->dependencies);
    entry->dependencies = new_list;
    cell_retain(entry->dependencies);
}

Cell* module_registry_get_dependencies(const char* module_name) {
    if (!module_name) return cell_nil();

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return cell_nil();

    cell_retain(entry->dependencies);
    return entry->dependencies;
}

void module_registry_free(void) {
    strtable_free(&registry.modules, free_module_entry);
    /* symbol_index values are entry->name pointers owned by entries — don't double-free */
    strtable_free(&registry.symbol_index, NULL);
    registry.count = 0;
}

void module_set_current_loading(const char* module_name) {
    current_loading_module = module_name;
}

const char* module_get_current_loading(void) {
    return current_loading_module;
}

/* Module versioning support */
void module_registry_set_version(const char* module_name, const char* version) {
    if (!module_name || !version) return;

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return;

    if (entry->version) free(entry->version);
    entry->version = strdup(version);
}

const char* module_registry_get_version(const char* module_name) {
    if (!module_name) return NULL;

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return NULL;
    return entry->version;
}

/* Selective exports */
void module_registry_set_exports(const char* module_name, Cell* exports) {
    if (!module_name) return;

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return;

    if (entry->exports) cell_release(entry->exports);
    if (entry->export_set) {
        strtable_free(entry->export_set, NULL);
        free(entry->export_set);
        entry->export_set = NULL;
    }

    if (exports) {
        cell_retain(exports);
        entry->exports = exports;

        /* Build O(1) export_set from Cell list */
        entry->export_set = malloc(sizeof(StrTable));
        strtable_init(entry->export_set, 16);

        Cell* curr = exports;
        while (curr && !cell_is_nil(curr)) {
            Cell* exp = cell_car(curr);
            if (cell_is_symbol(exp)) {
                const char* name = cell_get_symbol(exp);
                if (name[0] == ':') name++;
                strtable_put(entry->export_set, name, (void*)1);
            }
            curr = cell_cdr(curr);
        }
    } else {
        entry->exports = NULL;
    }
}

Cell* module_registry_get_exports(const char* module_name) {
    if (!module_name) return cell_nil();

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return cell_nil();

    if (entry->exports) {
        cell_retain(entry->exports);
        return entry->exports;
    }
    /* No explicit exports = export all symbols */
    cell_retain(entry->symbols);
    return entry->symbols;
}

int module_registry_is_exported(const char* module_name, const char* symbol) {
    if (!module_name || !symbol) return 0;

    ModuleEntry* entry = (ModuleEntry*)strtable_get(&registry.modules, module_name);
    if (!entry) return 0;

    /* No explicit exports = everything exported */
    if (!entry->export_set) return 1;

    /* Normalize symbol name */
    const char* search_name = symbol;
    if (symbol[0] == ':') search_name = symbol + 1;

    /* O(1) lookup */
    return strtable_get(entry->export_set, search_name) != NULL;
}

/* Cycle detection using DFS */
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
