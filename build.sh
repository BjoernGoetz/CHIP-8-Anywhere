#!/bin/sh
set -e
mkdir -p build
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -o build/chip8 platform/terminal/main.c
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -Ithird_party/unity -o build/test_core tests/test_core.c third_party/unity/unity.c
./build/test_core
