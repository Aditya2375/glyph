#!/bin/sh
# glyph test suite: engine unit checks, headless gameplay checks, and a
# golden render of a scripted snake session.
set -e
cd "$(dirname "$0")/.."
OUT=build/tests
mkdir -p "$OUT"

cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc \
    tests/test_engine.c build/libglyph.a -o "$OUT/test_engine"
"$OUT/test_engine"

cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DSNAKE_NO_MAIN \
    tests/test_snake.c examples/snake.c build/libglyph.a -o "$OUT/test_snake"
"$OUT/test_snake"

# golden render: 5 idle ticks of snake, dumped as text
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DSNAKE_NO_MAIN \
    tests/dump_frame.c examples/snake.c build/libglyph.a -o "$OUT/dump_frame"
"$OUT/dump_frame" > "$OUT/frame.txt"
diff tests/frame.expected "$OUT/frame.txt"
echo "RENDER TEST PASSED"
echo "ALL TESTS PASSED"
