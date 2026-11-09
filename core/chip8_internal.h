/*
 * Access to the core's internals, for the unit tests only.
 * Platforms must include only chip8.h.
 */
#ifndef CHIP8_INTERNAL_H
#define CHIP8_INTERNAL_H

#include "chip8.h"
#include <stdbool.h>
#include <stdint.h>

uint16_t chip8_read_pc(const struct chip8 *c);
void chip8_write_pc(struct chip8 *c, uint16_t pc);
uint8_t chip8_read_memory(const struct chip8 *c, uint16_t addr);
void chip8_write_memory(struct chip8 *c, uint16_t addr, uint8_t value);
bool chip8_read_pixel(const struct chip8 *c, uint8_t x, uint8_t y);
void chip8_write_pixel(struct chip8 *c, uint8_t x, uint8_t y, bool is_on);

#endif
