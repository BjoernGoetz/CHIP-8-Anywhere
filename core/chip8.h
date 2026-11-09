#ifndef CHIP8_H
#define CHIP8_H

#include <stddef.h>
#include <stdint.h>

struct chip8;
struct chip8 *chip8_create(void);
void chip8_load_rom(struct chip8 *c, const uint8_t *rom, size_t size);

#endif
