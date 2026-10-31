#!/bin/sh
# glyph test suite: engine unit checks, headless gameplay checks, and a
# golden render of a scripted snake session.
set -e
cd "$(dirname "$0")/.."
OUT=build/tests
mkdir -p "$OUT"

cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc $EXTRA \
    tests/test_engine.c build/libglyph.a -o "$OUT/test_engine"
"$OUT/test_engine"

cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DSNAKE_NO_MAIN $EXTRA \
    tests/test_snake.c examples/snake.c build/libglyph.a -o "$OUT/test_snake"
"$OUT/test_snake"

cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DSNAKE_NO_MAIN -DLIFE_NO_MAIN $EXTRA \
    tests/test_life.c examples/life.c build/libglyph.a -o "$OUT/test_life"
"$OUT/test_life"

# golden render: 5 idle ticks of snake, dumped as text
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DSNAKE_NO_MAIN $EXTRA \
    tests/dump_frame.c examples/snake.c build/libglyph.a -o "$OUT/dump_frame"
"$OUT/dump_frame" > "$OUT/frame.txt"
diff tests/frame.expected "$OUT/frame.txt"
echo "RENDER TEST PASSED"

# the real interactive loop on a pty: frames render, q exits cleanly
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DLIFE_NO_MAIN $EXTRA \
    tests/pty_test.c examples/life.c build/libglyph.a -lutil -o "$OUT/pty_test"
"$OUT/pty_test"

echo "ALL TESTS PASSED"
