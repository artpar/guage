CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O2
LDFLAGS =

# Source files
SOURCES = cell.c primitives.c debruijn.c debug.c eval.c cfg.c dfg.c pattern.c pattern_check.c type.c testgen.c module.c macro.c main.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = guage

# Build rules
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
cell.o: cell.c cell.h
primitives.o: primitives.c primitives.h cell.h pattern.h type.h testgen.h module.h
debruijn.o: debruijn.c debruijn.h cell.h
debug.o: debug.c debug.h cell.h eval.h
eval.o: eval.c eval.h cell.h primitives.h debruijn.h debug.h module.h macro.h
cfg.o: cfg.c cfg.h cell.h primitives.h
dfg.o: dfg.c dfg.h cell.h primitives.h
pattern.o: pattern.c pattern.h pattern_check.h cell.h eval.h
pattern_check.o: pattern_check.c pattern_check.h cell.h
type.o: type.c type.h
testgen.o: testgen.c testgen.h type.h cell.h
module.o: module.c module.h cell.h
macro.o: macro.c macro.h cell.h eval.h primitives.h
main.o: main.c cell.h primitives.h eval.h debug.h module.h

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

test: $(EXECUTABLE)
	@echo "Testing basic expressions..."
	@echo "(+ 1 2)" | ./$(EXECUTABLE)
	@echo "(* 3 4)" | ./$(EXECUTABLE)
	@echo "(cons 1 2)" | ./$(EXECUTABLE)

.PHONY: all clean run test
