CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O2
LDFLAGS =

# Source files
SOURCES = cell.c primitives.c debruijn.c debug.c eval.c cfg.c dfg.c pattern.c main.c
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
primitives.o: primitives.c primitives.h cell.h pattern.h
debruijn.o: debruijn.c debruijn.h cell.h
debug.o: debug.c debug.h cell.h eval.h
eval.o: eval.c eval.h cell.h primitives.h debruijn.h debug.h
cfg.o: cfg.c cfg.h cell.h primitives.h
dfg.o: dfg.c dfg.h cell.h primitives.h
pattern.o: pattern.c pattern.h cell.h eval.h
main.o: main.c cell.h primitives.h eval.h debug.h

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
