#!/bin/sh
set -e

#   6012  V0 = 0x12
#   A123  I = 0x123
#   00E0  clear display
#   E000  unknown opcode
printf '\140\022\241\043\000\340\340\000' > build/smoke.ch8

# The program must stop at E000 with exit code 1, and stderr must be
# exactly this one line: any sanitizer report (memory error, undefined
# behaviour, leak) adds output and fails the test.
status=0
./build/chip8_sanitized build/smoke.ch8 2> build/smoke_out.txt || status=$?
[ "$status" -eq 1 ]
echo "unknown opcode: E000" > build/smoke_expected.txt
cmp -s build/smoke_expected.txt build/smoke_out.txt
