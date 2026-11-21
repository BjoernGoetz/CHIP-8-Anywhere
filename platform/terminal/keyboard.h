#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "chip8.h"
#include <stdint.h>

void keyboard_init(void);
void keyboard_update(struct chip8 *c);

#endif
