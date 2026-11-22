/*
 * Drivers every platform must provide. The shared emulator calls them;
 * each platform defines them in its own files.
 */
#ifndef DRIVERS_H
#define DRIVERS_H

#include "chip8.h"
#include <stdbool.h>
#include <stdint.h>

uint64_t clock_microseconds(void);

void display_init(void);
void display_update(const struct chip8 *c);
void display_shutdown(void);

void keyboard_init(void);
void keyboard_update(struct chip8 *c);

void sound_init(void);
void sound_set_active(bool active);
void sound_shutdown(void);

#endif
