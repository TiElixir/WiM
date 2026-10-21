CC ?= cc
CPPFLAGS ?= -D_POSIX_C_SOURCE=200809L
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=
LDLIBS ?=

PROGRAM := wim
SOURCES := main.c editor.c terminal.c buffer.c
OBJECTS := $(SOURCES:.c=.o)

TEST_PROGRAM := tests/test_buffer
TEST_OBJECTS := tests/test_buffer.o buffer.o

.PHONY: all build test test-cli test-interactive check clean

all: build

build: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(TEST_PROGRAM): $(TEST_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tests/%.o: tests/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

test: $(TEST_PROGRAM)
	./$(TEST_PROGRAM)

test-cli: build
	sh tests/test_cli.sh

test-interactive: build
	sh tests/test_interactive.sh

check: build test test-cli test-interactive

clean:
	rm -f $(PROGRAM) $(OBJECTS) $(TEST_PROGRAM) $(TEST_OBJECTS) \
		$(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)
