#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CHIP8_DISPLAY_WIDTH 64
#define CHIP8_DISPLAY_HEIGHT 32

struct chip8;

enum chip8_step_status {
  CHIP8_STEP_OK,
  CHIP8_STEP_UNKNOWN_OPCODE,
};

struct chip8_step_result {
  enum chip8_step_status status;
  uint16_t opcode;
};

struct chip8 *chip8_create(void);
void chip8_destroy(struct chip8 *c);
void chip8_load_rom(struct chip8 *c, const uint8_t *rom, size_t size);
struct chip8_step_result chip8_step(struct chip8 *c);
void chip8_update_timers(struct chip8 *c, uint32_t elapsed_microseconds);
bool chip8_read_pixel(const struct chip8 *c, int x, int y);

#endif
