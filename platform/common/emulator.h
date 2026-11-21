/*
 * The part of the main loop that is the same on every platform. Each
 * platform owns the loop itself and calls emulator_step from it.
 */
#ifndef EMULATOR_H
#define EMULATOR_H

#include "chip8.h"

void emulator_init(void);
struct chip8_step_result emulator_step(struct chip8 *c);
void emulator_shutdown(void);

#endif
