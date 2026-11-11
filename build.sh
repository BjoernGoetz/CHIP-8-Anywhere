#!/bin/sh
set -e
mkdir -p build

CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -Werror -Icore"
SANITIZE="-fsanitize=address,undefined -fno-sanitize-recover=all"

CORE="core/chip8.c"
TERMINAL="platform/terminal/main.c platform/terminal/display.c"
UNITY="-Ithird_party/unity third_party/unity/unity.c"

gcc $CFLAGS -o build/chip8 $TERMINAL $CORE
gcc $CFLAGS $SANITIZE -o build/chip8_sanitized $TERMINAL $CORE
gcc $CFLAGS $SANITIZE $UNITY -o build/test_chip8 tests/core/test_chip8.c $CORE
gcc $CFLAGS $SANITIZE $UNITY -Iplatform/terminal -o build/test_display \
    tests/platform/terminal/test_display.c platform/terminal/display.c $CORE

./build/test_chip8
./build/test_display
./tests/platform/terminal/smoke.sh
