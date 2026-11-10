#include "chip8.h"
#include "chip8_internal.h"
#include "chip8_platform.h"
#include <stdint.h>

#define PROGRAM_START 0x200
#define DISPLAY_HEIGHT 32
#define DISPLAY_WIDTH 64

struct chip8 *chip8_create(void) {
  struct chip8 *c = chip8_platform_alloc(sizeof(struct chip8));
  c->pc = PROGRAM_START;
  return c;
}

void chip8_destroy(struct chip8 *c) { chip8_platform_free(c); }

void chip8_load_rom(struct chip8 *c, const uint8_t *rom, size_t size) {
  for (size_t i = 0; i < size; i++) {
    c->memory[PROGRAM_START + i] = rom[i];
  }
}

struct chip8_step_result chip8_step(struct chip8 *c) {
  uint16_t opcode = (uint16_t)(c->memory[c->pc] << 8 | c->memory[c->pc + 1]);
  c->pc += 2;

  if (opcode == 0x00E0) {
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
      for (int x = 0; x < DISPLAY_WIDTH; x++) {
        c->display[y][x] = false;
      }
    }

    return (struct chip8_step_result){CHIP8_STEP_OK, 0};
  }
  if ((opcode & 0xF000) == 0xA000) {
    c->i = opcode & 0x0FFF;
    return (struct chip8_step_result){CHIP8_STEP_OK, 0};
  }
  if ((opcode & 0xF000) == 0x6000) {
    c->v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
    return (struct chip8_step_result){CHIP8_STEP_OK, 0};
  }

  return (struct chip8_step_result){CHIP8_STEP_UNKNOWN_OPCODE, opcode};
}
