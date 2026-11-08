#include "chip8.h"
#include "chip8_internal.h"
#include "chip8_platform.h"
#include <stdint.h>

struct chip8 {
  uint8_t memory[4096];
};

struct chip8 *chip8_create(void) { return NULL; }

size_t chip8_memory_size(const struct chip8 *c) { return sizeof c->memory; }
