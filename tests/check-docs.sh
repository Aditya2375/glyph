#!/bin/sh
# Doc-claims guard: check counts quoted in README.md must match the suites.
set -e
cd "$(dirname "$0")/.."
OUT=build/docs-check
mkdir -p "$OUT"
# build from source, not the archive: the archive may carry sanitizer
# flags from another job's build
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc \
    tests/test_engine.c src/glyph.c -o "$OUT/test_engine"
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DSNAKE_NO_MAIN \
    tests/test_snake.c examples/snake.c src/glyph.c -o "$OUT/test_snake"
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -Iexamples -DSNAKE_NO_MAIN -DLIFE_NO_MAIN \
    tests/test_life.c examples/life.c src/glyph.c -o "$OUT/test_life"

ENGINE=$("$OUT/test_engine" | grep -o '^[0-9]* checks passed' | grep -o '^[0-9]*')
SNAKE=$("$OUT/test_snake" | grep -o '^[0-9]* checks passed' | grep -o '^[0-9]*')
LIFE=$("$OUT/test_life" | grep -o '^[0-9]* checks passed' | grep -o '^[0-9]*')
TOTAL=$((ENGINE + SNAKE + LIFE))

check_count() {
    stated=$(grep -o "$1" README.md | head -1 | grep -o '^[0-9]*')
    [ "$stated" = "$2" ] || { echo "DOC DRIFT: README says $stated, suite runs $2 ($3)"; exit 1; }
}
check_count '[0-9]* engine unit checks' "$ENGINE" "engine"
check_count '[0-9]* headless gameplay checks' "$SNAKE" "snake"
check_count '[0-9]* Game of Life checks' "$LIFE" "life"
STATED_TOTAL=$(grep -o 'runs [0-9]* checks' README.md | grep -o '[0-9]*')
[ "$STATED_TOTAL" = "$TOTAL" ] || { echo "DOC DRIFT: README says $STATED_TOTAL total checks, suites run $TOTAL"; exit 1; }
echo "doc counts verified: $TOTAL checks ($ENGINE engine, $SNAKE snake, $LIFE life)"
