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
  enum chip8_step_status status = CHIP8_STEP_OK;
  uint16_t opcode = (uint16_t)(c->memory[c->pc] << 8 | c->memory[c->pc + 1]);
  c->pc += 2;

  if (opcode == 0x00E0) {
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
      for (int x = 0; x < DISPLAY_WIDTH; x++) {
        c->display[y][x] = false;
      }
    }
  } else if ((opcode & 0xF000) == 0xA000) {
    c->i = opcode & 0x0FFF;
  } else if ((opcode & 0xF000) == 0x6000) {
    c->v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
  } else if ((opcode & 0xD000) == 0xD000) {
    uint8_t x_reg = (opcode & 0x0F00) >> 8;
    uint8_t y_reg = (opcode & 0x00F0) >> 4;
    uint8_t x_offset = c->v[x_reg];
    uint8_t y_offset = c->v[y_reg];
    uint8_t sprite = c->memory[c->i];

    for (int x = 0; x < 8; x++) {
      c->display[y_offset][x_offset + x] = sprite >> (7 - x) & 1;
    }
  } else if ((opcode & 0xF000) == 0x7000) {
    uint8_t x_reg = (opcode & 0x0F00) >> 8;
    uint16_t nn = opcode & 0x00FF;
    c->v[x_reg] += nn;
  } else {
    status = CHIP8_STEP_UNKNOWN_OPCODE;
  }

  return (struct chip8_step_result){status, opcode};
}
