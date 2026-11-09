/*
 * Access to the core's internals, for the unit tests only.
 * Platforms must include only chip8.h.
 */
#ifndef CHIP8_INTERNAL_H
#define CHIP8_INTERNAL_H

#include "chip8.h"
#include <stdint.h>

uint16_t chip8_read_pc(const struct chip8 *c);

#endif
