#include "chip8.h"
#include "chip8_internal.h"
#include "chip8_platform.h"
#include <stdint.h>

#define PROGRAM_START 0x200
#define DISPLAY_HEIGHT 32
#define DISPLAY_WIDTH 64

struct chip8 {
  uint8_t memory[4096];
  uint16_t pc;
  uint16_t i;
  bool display[64][32];
};

struct chip8 *chip8_create(void) {
  struct chip8 *c = chip8_platform_alloc(sizeof(struct chip8));
  c->pc = PROGRAM_START;
  return c;
}

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

  return (struct chip8_step_result){CHIP8_STEP_UNKNOWN_OPCODE, opcode};
}

uint16_t chip8_read_pc(const struct chip8 *c) { return c->pc; }

void chip8_write_pc(struct chip8 *c, uint16_t pc) { c->pc = pc; }

uint16_t chip8_read_i(const struct chip8 *c) { return c->i; }

uint8_t chip8_read_memory(const struct chip8 *c, uint16_t addr) {
  return c->memory[addr];
}

void chip8_write_memory(struct chip8 *c, uint16_t addr, uint8_t value) {
  c->memory[addr] = value;
}

bool chip8_read_pixel(const struct chip8 *c, uint8_t x, uint8_t y) {
  return c->display[y][x];
}

void chip8_write_pixel(struct chip8 *c, uint8_t x, uint8_t y, bool is_on) {
  c->display[y][x] = is_on;
}
