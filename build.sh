#!/bin/sh
set -e
mkdir build
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -o build/chip8 platform/terminal/main.c
