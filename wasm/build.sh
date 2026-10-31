#!/bin/sh
# Rebuild the browser demo in docs/ (requires emsdk on PATH).
set -e
cd "$(dirname "$0")/.."
emcc -O2 -Isrc src/glyph.c examples/snake.c wasm/glyph_wasm.c -o docs/glyph.js \
    -sEXPORTED_FUNCTIONS=_glyph_snake_new,_glyph_snake_free,_glyph_key,_glyph_tick,_glyph_w,_glyph_h,_glyph_cell \
    -sEXPORTED_RUNTIME_METHODS=ccall -sMODULARIZE=1 -sEXPORT_NAME=GlyphModule -sALLOW_MEMORY_GROWTH=1
