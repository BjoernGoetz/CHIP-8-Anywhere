#include "chip8.h"
#include "chip8_internal.h"
#include "chip8_platform.h"
#include <stdint.h>

#define PROGRAM_START 0x200

struct chip8 {
  uint8_t memory[4096];
  uint16_t pc;
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
  return (struct chip8_step_result){CHIP8_STEP_UNKNOWN_OPCODE, opcode};
}

uint16_t chip8_read_pc(const struct chip8 *c) { return c->pc; }

uint8_t chip8_read_memory(const struct chip8 *c, uint16_t addr) {
  return c->memory[addr];
}
