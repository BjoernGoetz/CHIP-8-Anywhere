/*
 * Access to the core's internals, for the unit tests only.
 * Platforms must include only chip8.h.
 */
#ifndef CHIP8_INTERNAL_H
#define CHIP8_INTERNAL_H

#include "chip8.h"
#include <stddef.h>

size_t chip8_memory_size(const struct chip8 *c);

#endif
