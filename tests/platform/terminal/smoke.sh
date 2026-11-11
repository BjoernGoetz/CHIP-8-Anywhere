#!/bin/sh
set -e

current_test=""
trap 'code=$?; [ "$code" -eq 0 ] || echo "smoke: $current_test failed" >&2' EXIT

run_test() {
    current_test=$1
    "$1"
}

run_chip8() {
    status=0
    ./build/chip8_sanitized "$@" > build/smoke_screen.txt 2> build/smoke_out.txt || status=$?
}

expect_stderr() {
    echo "$1" > build/smoke_expected.txt
    cmp -s build/smoke_expected.txt build/smoke_out.txt
}

# Writes a ROM that draws one sprite row, then stops at an unknown opcode:
#   00E0  clear display
#   6000  V0 = 0
#   6100  V1 = 0
#   A20C  I = 0x20C, the sprite at the end
#   D011  draw 1 row at (V0, V1)
#   E000  unknown opcode
#   A5    sprite: 1010 0101
write_sprite_rom() {
    printf '\000\340\140\000\141\000\242\014\320\021\340\000\245' > build/smoke.ch8
}

test_stops_at_unknown_opcode() {
    write_sprite_rom
    run_chip8 build/smoke.ch8
    [ "$status" -eq 1 ]
    expect_stderr "unknown opcode: E000"
}

test_draws_last_frame() {
    write_sprite_rom
    run_chip8 build/smoke.ch8
    tail -n 32 build/smoke_screen.txt | head -n 1 | grep -q '██  ██    ██  ██  '
    [ "$(tail -n 31 build/smoke_screen.txt | grep -c '█')" -eq 0 ]
}

test_reports_missing_rom_file() {
    run_chip8 build/missing.ch8
    [ "$status" -eq 1 ]
    expect_stderr "cannot open ROM 'build/missing.ch8'"
}

test_prints_usage_without_rom_path() {
    run_chip8
    [ "$status" -eq 2 ]
    expect_stderr "usage: ./build/chip8_sanitized <rom.ch8>"
}

run_test test_stops_at_unknown_opcode
run_test test_draws_last_frame
run_test test_reports_missing_rom_file
run_test test_prints_usage_without_rom_path
