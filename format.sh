#!/bin/sh
set -e
files=$(find core platform tests -name '*.c' -o -name '*.h')
if [ "$1" = "--check" ]; then
    clang-format --dry-run --Werror $files
else
    clang-format -i $files
fi
