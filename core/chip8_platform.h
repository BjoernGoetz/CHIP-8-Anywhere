/*
 * Functions every platform must provide. The core declares them here and
 * calls them; each platform (and the tests) defines them.
 */
#ifndef CHIP8_PLATFORM_H
#define CHIP8_PLATFORM_H

#include <stddef.h>

void *chip8_platform_alloc(size_t size);

#endif
