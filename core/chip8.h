#ifndef CHIP8_H
#define CHIP8_H

#include <stddef.h>
#include <stdint.h>

struct chip8;

enum chip8_step_result {
  CHIP8_STEP_OK,
  CHIP8_STEP_UNKNOWN_OPCODE,
};

struct chip8 *chip8_create(void);
void chip8_load_rom(struct chip8 *c, const uint8_t *rom, size_t size);
enum chip8_step_result chip8_step(struct chip8 *c);

#endif
