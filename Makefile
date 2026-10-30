CC      ?= cc
CFLAGS  ?= -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc

all: build/libglyph.a build/snake

build/libglyph.a: src/glyph.c src/glyph.h
	@mkdir -p build
	$(CC) $(CFLAGS) -c src/glyph.c -o build/glyph.o
	ar rcs $@ build/glyph.o

build/snake: examples/snake.c build/libglyph.a
	$(CC) $(CFLAGS) -Iexamples examples/snake.c build/libglyph.a -o $@

check: all
	./tests/run-tests.sh

clean:
	rm -rf build

.PHONY: all check clean
