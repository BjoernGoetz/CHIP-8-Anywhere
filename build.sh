#!/bin/sh
set -e
mkdir -p build
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -o build/chip8 platform/terminal/main.c
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -Icore -Ithird_party/unity -o build/test_chip8 tests/test_chip8.c core/chip8.c third_party/unity/unity.c
./build/test_chip8
