#include "emulator.h"
#include "drivers.h"

static uint64_t last_microseconds;

void emulator_init(void) {
  display_init();
  sound_init();
  keyboard_init();
  last_microseconds = clock_microseconds();
}

struct chip8_step_result emulator_step(struct chip8 *c) {
  struct chip8_step_result r = chip8_step(c);

  uint64_t now = clock_microseconds();
  chip8_update_timers(c, (uint32_t)(now - last_microseconds));
  last_microseconds = now;

  keyboard_update(c);
  sound_set_active(chip8_is_sound_active(c));
  display_update(c);

  return r;
}

void emulator_shutdown(void) {
  sound_shutdown();
  display_shutdown();
}
