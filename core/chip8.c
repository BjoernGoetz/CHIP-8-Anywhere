#include "chip8.h"
#include "chip8_internal.h"
#include "chip8_platform.h"
#include <stdint.h>

struct chip8 {
  uint8_t memory[4096];
  uint16_t pc;
};

struct chip8 *chip8_create(void) {
  struct chip8 *c = chip8_platform_alloc(sizeof(struct chip8));
  c->pc = 0x200;
  return c;
}

uint16_t chip8_read_pc(const struct chip8 *c) { return c->pc; }
