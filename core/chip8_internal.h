/*
 * The core's internals, for chip8.c and the unit tests only.
 * Platforms must include only chip8.h.
 */
#ifndef CHIP8_INTERNAL_H
#define CHIP8_INTERNAL_H

#include "chip8.h"
#include <stdbool.h>
#include <stdint.h>

struct chip8 {
  uint8_t memory[4096];
  uint16_t pc;
  uint16_t i;
  uint8_t v[16];
  bool display[32][64];
};

#endif
