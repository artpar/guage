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

void module_registry_free(void) {
    ModuleEntry* entry = registry.head;
    while (entry) {
        ModuleEntry* next = entry->next;
        free(entry->name);
        cell_release(entry->symbols);
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
