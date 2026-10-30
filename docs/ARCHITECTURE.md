# glyph architecture

## Layers

```text
game code (update/draw) -> engine loop -> screen grid -> ANSI diff to the terminal
                          (glyph.c)       (screen)      (flush)
```

A game supplies two callbacks and owns its state struct; the engine owns
time, input, and presentation. Nothing in the engine knows what a snake is.

## The screen

Two cell grids (front = what the terminal shows, back = what the game just
drew). `screen_flush` walks both, and for each changed cell emits a cursor
move plus the character, batching color escapes so identical runs cost one
escape sequence. Unchanged cells emit nothing - a mostly-static frame is a
handful of bytes.

## Input

A 64-entry ring of key codes. `input_read_fd` does one non-blocking read
and decodes CSI escape sequences (`ESC [ A` ...) into arrow codes;
everything else passes through as its byte. A full queue drops new keys
rather than blocking the loop.

## The loop and headless mode

`game_run` puts the terminal in raw mode (no canonical input, no echo,
hidden cursor), then runs fixed-timestep frames: drain input, update,
draw, flush, sleep out the remainder of `1000/fps` ms. On exit it restores
the terminal exactly.

`game_run_headless` runs the identical update/draw pair with a scripted
key per tick and no sleeping, no terminal, no ANSI. This is the engine's
testing contract: gameplay is deterministic, so the suite can drive a real
game - steer the snake onto food, assert it grew, run it into a wall,
assert the game ended - without a display.
