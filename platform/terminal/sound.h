#ifndef SOUND_H
#define SOUND_H

#include <stdbool.h>

void sound_init(void);
void sound_set_active(bool active);
void sound_shutdown(void);

#endif
