CC      ?= cc
CFLAGS  ?= -O2 -g
# required flags survive a user/environment CFLAGS; extras go in EXTRA
override CFLAGS += -std=gnu11 -Wall -Wextra -Werror -Isrc $(EXTRA)

all: build/libglyph.a build/snake build/life

build/libglyph.a: src/glyph.c src/glyph.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c src/glyph.c -o build/glyph.o
	ar rcs $@ build/glyph.o

build/snake: examples/snake.c build/libglyph.a
	$(CC) $(CFLAGS) -Iexamples examples/snake.c build/libglyph.a -o $@

build/life: examples/life.c build/libglyph.a
	$(CC) $(CFLAGS) -Iexamples examples/life.c build/libglyph.a -o $@

check: all
	./tests/run-tests.sh

clean:
	rm -rf build

.PHONY: all check clean
