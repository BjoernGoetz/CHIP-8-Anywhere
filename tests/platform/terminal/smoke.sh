#!/bin/sh
set -e

#   00E0  clear display
#   6000  V0 = 0
#   6100  V1 = 0
#   A20C  I = 0x20C, the sprite at the end
#   D011  draw 1 row at (V0, V1)
#   E000  unknown opcode
#   A5    sprite: 1010 0101
printf '\000\340\140\000\141\000\242\014\320\021\340\000\245' > build/smoke.ch8

# The program must stop at E000 with exit code 1, and stderr must be
# exactly this one line: any sanitizer report (memory error, undefined
# behaviour, leak) adds output and fails the test.
status=0
./build/chip8_sanitized build/smoke.ch8 > build/smoke_screen.txt 2> build/smoke_out.txt || status=$?
[ "$status" -eq 1 ]
echo "unknown opcode: E000" > build/smoke_expected.txt
cmp -s build/smoke_expected.txt build/smoke_out.txt

# The last frame is the last 32 lines: its first row shows the sprite,
# two characters per pixel, and the other 31 rows show nothing.
tail -n 32 build/smoke_screen.txt | head -n 1 | grep -q '██  ██    ██  ██  '
[ "$(tail -n 31 build/smoke_screen.txt | grep -c '█')" -eq 0 ]
