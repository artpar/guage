# Guage Language - Root Makefile
# Build, test, and run the Guage programming language

# Project directories
BOOTSTRAP_DIR = bootstrap
BUILD_DIR = $(BOOTSTRAP_DIR)
STDLIB_DIR = $(BOOTSTRAP_DIR)/stdlib
TESTS_DIR = $(BOOTSTRAP_DIR)/tests

# Build configuration
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O2 -fno-omit-frame-pointer
LDFLAGS = -Wl,-stack_size,0x2000000  # 32MB stack (increased from default 8MB)

# Source files (all in bootstrap/)
SOURCES = cell.c primitives.c debruijn.c debug.c eval.c cfg.c dfg.c \
          pattern.c pattern_check.c type.c testgen.c module.c macro.c \
          fiber.c actor.c linenoise.c main.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = guage

# Add bootstrap/ prefix to paths
BOOTSTRAP_SOURCES = $(addprefix $(BOOTSTRAP_DIR)/, $(SOURCES))
BOOTSTRAP_OBJECTS = $(addprefix $(BOOTSTRAP_DIR)/, $(OBJECTS))
BOOTSTRAP_EXECUTABLE = $(BOOTSTRAP_DIR)/$(EXECUTABLE)

# ============================================================================
# Main targets
# ============================================================================

.PHONY: all build clean test repl help install uninstall

# Default target
all: build

# Build the Guage interpreter
build: $(BOOTSTRAP_EXECUTABLE)

$(BOOTSTRAP_EXECUTABLE): $(BOOTSTRAP_OBJECTS)
	@echo "Linking $(EXECUTABLE)..."
	$(CC) $(LDFLAGS) $(BOOTSTRAP_OBJECTS) -o $@
	@echo "Build complete! Run 'make repl' to start."

# Compile C source files
$(BOOTSTRAP_DIR)/%.o: $(BOOTSTRAP_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# Testing targets
# ============================================================================

# Run full test suite (33 Guage tests)
test: build
	@echo "Running full Guage test suite..."
	@$(BOOTSTRAP_DIR)/run_tests.sh

# Run a single test file (usage: make test-one TEST=bootstrap/tests/basic.test)
test-one: build
	@if [ -z "$(TEST)" ]; then \
		echo "Usage: make test-one TEST=bootstrap/tests/basic.test"; \
		exit 1; \
	fi
	@echo "Running single test: $(TEST)"
	@$(BOOTSTRAP_EXECUTABLE) < $(TEST)

# Quick smoke test (just verify interpreter works)
smoke: build
	@echo "Running smoke tests..."
	@echo "(⊕ #1 #2)" | $(BOOTSTRAP_EXECUTABLE)
	@echo "(⊗ #3 #4)" | $(BOOTSTRAP_EXECUTABLE)
	@echo "(⟨⟩ #1 #2)" | $(BOOTSTRAP_EXECUTABLE)
	@echo "✓ Smoke tests passed!"

# View test results summary (quick overview)
test-summary: build
	@echo "Running tests and showing summary only..."
	@$(BOOTSTRAP_DIR)/run_tests.sh 2>&1 | grep -A 10 "Test Summary"

# ============================================================================
# Running targets
# ============================================================================

# Start the REPL (from project root)
repl: build
	@echo "Starting Guage REPL from project root..."
	@echo "Note: Use 'bootstrap/stdlib/...' paths in (⋘ ...) commands"
	@echo "Type :help for help, :quit to exit"
	@echo ""
	@$(BOOTSTRAP_EXECUTABLE)

# Run a specific file
run: build
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make run FILE=path/to/file.scm"; \
		exit 1; \
	fi
	@$(BOOTSTRAP_EXECUTABLE) < $(FILE)

# ============================================================================
# Cleaning targets
# ============================================================================

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(BOOTSTRAP_OBJECTS) $(BOOTSTRAP_EXECUTABLE)
	@echo "Clean complete!"

# Deep clean (including temp files, backups)
distclean: clean
	@echo "Deep cleaning..."
	@find . -name "*.o" -delete
	@find . -name "*.bak" -delete
	@find . -name "*.bak[0-9]" -delete
	@find . -name "*.tmp" -delete
	@find . -name "*.temp" -delete
	@find . -name "*_temp.*" -delete
	@find . -name ".DS_Store" -delete
	@echo "Deep clean complete!"

# ============================================================================
# Installation targets (future)
# ============================================================================

# Install to system (future implementation)
install: build
	@echo "Installation not yet implemented"
	@echo "To use Guage, run: ./bootstrap/guage"

# Uninstall from system (future implementation)
uninstall:
	@echo "Uninstallation not yet implemented"

# ============================================================================
# Development targets
# ============================================================================

# Rebuild everything from scratch
rebuild: clean build

# Show build info
info:
	@echo "Guage Build Information"
	@echo "======================="
	@echo "Compiler:      $(CC)"
	@echo "CFLAGS:        $(CFLAGS)"
	@echo "LDFLAGS:       $(LDFLAGS)"
	@echo "Sources:       $(words $(SOURCES)) files"
	@echo "Bootstrap dir: $(BOOTSTRAP_DIR)"
	@echo "Stdlib dir:    $(STDLIB_DIR)"
	@echo "Tests dir:     $(TESTS_DIR)"
	@echo ""
	@echo "Build status:"
	@if [ -f "$(BOOTSTRAP_EXECUTABLE)" ]; then \
		echo "  ✓ Executable built"; \
	else \
		echo "  ✗ Not built (run 'make build')"; \
	fi

# Help - show available targets
help:
	@echo "Guage Language - Available Make Targets"
	@echo "========================================"
	@echo ""
	@echo "Building:"
	@echo "  make              - Build the interpreter (default)"
	@echo "  make build        - Build the interpreter"
	@echo "  make rebuild      - Clean and rebuild from scratch"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Deep clean (includes temp files)"
	@echo ""
	@echo "Testing:"
	@echo "  make test         - Run full test suite (33 tests)"
	@echo "  make test-one TEST=path - Run a single test file"
	@echo "  make test-summary - Show only test results summary"
	@echo "  make smoke        - Quick smoke test"
	@echo ""
	@echo "Running:"
	@echo "  make repl         - Start the REPL"
	@echo "  make run FILE=x   - Run a specific file"
	@echo ""
	@echo "Information:"
	@echo "  make info         - Show build configuration"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Future:"
	@echo "  make install      - Install to system (not yet implemented)"
	@echo "  make uninstall    - Remove from system (not yet implemented)"
	@echo ""
	@echo "Quick start: make && make repl"

# ============================================================================
# Dependencies (auto-generated from bootstrap/Makefile)
# ============================================================================

$(BOOTSTRAP_DIR)/cell.o: $(BOOTSTRAP_DIR)/cell.c $(BOOTSTRAP_DIR)/cell.h
$(BOOTSTRAP_DIR)/primitives.o: $(BOOTSTRAP_DIR)/primitives.c $(BOOTSTRAP_DIR)/primitives.h \
                                $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/pattern.h \
                                $(BOOTSTRAP_DIR)/type.h $(BOOTSTRAP_DIR)/testgen.h \
                                $(BOOTSTRAP_DIR)/module.h $(BOOTSTRAP_DIR)/actor.h
$(BOOTSTRAP_DIR)/debruijn.o: $(BOOTSTRAP_DIR)/debruijn.c $(BOOTSTRAP_DIR)/debruijn.h \
                              $(BOOTSTRAP_DIR)/cell.h
$(BOOTSTRAP_DIR)/debug.o: $(BOOTSTRAP_DIR)/debug.c $(BOOTSTRAP_DIR)/debug.h \
                           $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/eval.h
$(BOOTSTRAP_DIR)/eval.o: $(BOOTSTRAP_DIR)/eval.c $(BOOTSTRAP_DIR)/eval.h \
                          $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/primitives.h \
                          $(BOOTSTRAP_DIR)/debruijn.h $(BOOTSTRAP_DIR)/debug.h \
                          $(BOOTSTRAP_DIR)/module.h $(BOOTSTRAP_DIR)/macro.h \
                          $(BOOTSTRAP_DIR)/fiber.h
$(BOOTSTRAP_DIR)/cfg.o: $(BOOTSTRAP_DIR)/cfg.c $(BOOTSTRAP_DIR)/cfg.h \
                         $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/primitives.h
$(BOOTSTRAP_DIR)/dfg.o: $(BOOTSTRAP_DIR)/dfg.c $(BOOTSTRAP_DIR)/dfg.h \
                         $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/primitives.h
$(BOOTSTRAP_DIR)/pattern.o: $(BOOTSTRAP_DIR)/pattern.c $(BOOTSTRAP_DIR)/pattern.h \
                             $(BOOTSTRAP_DIR)/pattern_check.h $(BOOTSTRAP_DIR)/cell.h \
                             $(BOOTSTRAP_DIR)/eval.h
$(BOOTSTRAP_DIR)/pattern_check.o: $(BOOTSTRAP_DIR)/pattern_check.c \
                                   $(BOOTSTRAP_DIR)/pattern_check.h $(BOOTSTRAP_DIR)/cell.h
$(BOOTSTRAP_DIR)/type.o: $(BOOTSTRAP_DIR)/type.c $(BOOTSTRAP_DIR)/type.h
$(BOOTSTRAP_DIR)/testgen.o: $(BOOTSTRAP_DIR)/testgen.c $(BOOTSTRAP_DIR)/testgen.h \
                             $(BOOTSTRAP_DIR)/type.h $(BOOTSTRAP_DIR)/cell.h
$(BOOTSTRAP_DIR)/module.o: $(BOOTSTRAP_DIR)/module.c $(BOOTSTRAP_DIR)/module.h \
                            $(BOOTSTRAP_DIR)/cell.h
$(BOOTSTRAP_DIR)/macro.o: $(BOOTSTRAP_DIR)/macro.c $(BOOTSTRAP_DIR)/macro.h \
                           $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/eval.h \
                           $(BOOTSTRAP_DIR)/primitives.h
$(BOOTSTRAP_DIR)/fiber.o: $(BOOTSTRAP_DIR)/fiber.c $(BOOTSTRAP_DIR)/fiber.h \
                           $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/eval.h
$(BOOTSTRAP_DIR)/actor.o: $(BOOTSTRAP_DIR)/actor.c $(BOOTSTRAP_DIR)/actor.h \
                           $(BOOTSTRAP_DIR)/cell.h $(BOOTSTRAP_DIR)/fiber.h \
                           $(BOOTSTRAP_DIR)/eval.h
$(BOOTSTRAP_DIR)/linenoise.o: $(BOOTSTRAP_DIR)/linenoise.c $(BOOTSTRAP_DIR)/linenoise.h
$(BOOTSTRAP_DIR)/main.o: $(BOOTSTRAP_DIR)/main.c $(BOOTSTRAP_DIR)/cell.h \
                          $(BOOTSTRAP_DIR)/primitives.h $(BOOTSTRAP_DIR)/eval.h \
                          $(BOOTSTRAP_DIR)/debug.h $(BOOTSTRAP_DIR)/module.h \
                          $(BOOTSTRAP_DIR)/linenoise.h
