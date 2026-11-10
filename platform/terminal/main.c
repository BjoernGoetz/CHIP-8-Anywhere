#include "chip8.h"
#include "chip8_platform.h"
#include <stdio.h>
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }
void chip8_platform_free(void *ptr) { free(ptr); }

int main(int argc, char **argv) {
  (void)argc;

  uint8_t rom[512];
  FILE *f = fopen(argv[1], "rb");
  size_t size = fread(rom, 1, sizeof rom, f);
  fclose(f);

  struct chip8 *c = chip8_create();
  chip8_load_rom(c, rom, size);

  struct chip8_step_result r;
  do {
    r = chip8_step(c);
  } while (r.status == CHIP8_STEP_OK);

  fprintf(stderr, "unknown opcode: %04X\n", r.opcode);
  chip8_destroy(c);

  return 1;
}
