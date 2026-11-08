#!/bin/sh
set -e
mkdir -p build
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -o build/chip8 platform/terminal/main.c
