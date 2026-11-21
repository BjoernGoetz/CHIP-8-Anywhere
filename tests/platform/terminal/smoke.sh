#!/bin/sh
set -e

export CHIP8_AUDIO_DUMP=/dev/null

current_test=""
trap 'code=$?; [ "$code" -eq 0 ] || echo "smoke: $current_test failed" >&2' EXIT

run_test() {
    current_test=$1
    "$1"
}

run_chip8() {
    status=0
    ./build/chip8_sanitized "$@" < /dev/null > build/smoke_screen.txt 2> build/smoke_out.txt || status=$?
}

run_chip8_with_timeout() {
    ./build/chip8_sanitized "$1" < "${2:-/dev/null}" > /dev/null 2> build/smoke_out.txt &
    pid=$!
    ( sleep 2; kill "$pid" 2> /dev/null ) > /dev/null 2>&1 &
    watchdog=$!
    status=0
    wait "$pid" 2> /dev/null || status=$?
    kill "$watchdog" 2> /dev/null || true
    wait "$watchdog" 2> /dev/null || true
}

run_chip8_with_audio_dump() {
    rm -f build/smoke_audio.raw
    CHIP8_AUDIO_DUMP=build/smoke_audio.raw
    run_chip8_with_timeout "$1"
    CHIP8_AUDIO_DUMP=/dev/null
}

count_nonzero_audio_bytes() {
    LC_ALL=C tr -d '\000' < build/smoke_audio.raw | wc -c | tr -d ' '
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

# Writes a ROM that sets the delay timer to 6 (0.1 s at 60 Hz), waits for it
# to reach 0, then stops at an unknown opcode:
#   6006  V0 = 6
#   F015  DT = V0
#   F007  V0 = DT                 <- loop
#   3000  skip next if V0 == 0
#   1204  jump to loop
#   E000  unknown opcode
write_delay_timer_rom() {
    printf '\140\006\360\025\360\007\060\000\022\004\340\000' > build/smoke.ch8
}

# Writes a ROM that plays a tone while waiting 0.1 s, then stops at an
# unknown opcode:
#   6006  V0 = 6
#   F018  ST = V0
#   F015  DT = V0
#   F007  V0 = DT                 <- loop
#   3000  skip next if V0 == 0
#   1206  jump to loop
#   E000  unknown opcode
write_sound_rom() {
    printf '\140\006\360\030\360\025\360\007\060\000\022\006\340\000' > build/smoke.ch8
}

# Writes a ROM that waits until key 5 is pressed, then stops at an unknown
# opcode:
#   6005  V0 = 5
#   E0A1  skip next if key V0 is not pressed    <- loop
#   1208  jump to the end
#   1202  jump to loop
#   E000  unknown opcode
write_wait_for_key_rom() {
    printf '\140\005\340\241\022\010\022\002\340\000' > build/smoke.ch8
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

test_ticks_timers() {
    write_delay_timer_rom
    run_chip8_with_timeout build/smoke.ch8
    expect_stderr "unknown opcode: E000"
}

test_plays_tone_while_sound_active() {
    write_sound_rom
    run_chip8_with_audio_dump build/smoke.ch8
    expect_stderr "unknown opcode: E000"
    [ "$(count_nonzero_audio_bytes)" -gt 0 ]
}

test_stays_silent_without_sound() {
    write_delay_timer_rom
    run_chip8_with_audio_dump build/smoke.ch8
    expect_stderr "unknown opcode: E000"
    [ -s build/smoke_audio.raw ]
    [ "$(count_nonzero_audio_bytes)" -eq 0 ]
}

test_reads_keys_from_stdin() {
    write_wait_for_key_rom
    rm -f build/smoke_keys.fifo
    mkfifo build/smoke_keys.fifo
    ( printf 'w'; sleep 3 ) > build/smoke_keys.fifo &
    writer=$!
    run_chip8_with_timeout build/smoke.ch8 build/smoke_keys.fifo
    kill "$writer" 2> /dev/null || true
    wait "$writer" 2> /dev/null || true
    expect_stderr "unknown opcode: E000"
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
run_test test_ticks_timers
run_test test_plays_tone_while_sound_active
run_test test_stays_silent_without_sound
run_test test_reads_keys_from_stdin
run_test test_reports_missing_rom_file
run_test test_prints_usage_without_rom_path
