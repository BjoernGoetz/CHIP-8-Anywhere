#include "chip8.h"
#include "chip8_platform.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }
void chip8_platform_free(void *ptr) { free(ptr); }

static size_t read_rom_file(const char *path, uint8_t *rom, size_t max) {
  FILE *f = fopen(path, "rb");
  size_t size = fread(rom, 1, max, f);
  fclose(f);

  return size;
}

int main(int argc, char **argv) {
  (void)argc;

  uint8_t rom[512];
  size_t size = read_rom_file(argv[1], rom, sizeof rom);

  struct chip8 *c = chip8_create();
  chip8_load_rom(c, rom, size);

  struct chip8_step_result r;
  do {
    r = chip8_step(c);
    display_draw(c, stdout);
  } while (r.status == CHIP8_STEP_OK);

  fprintf(stderr, "unknown opcode: %04X\n", r.opcode);
  chip8_destroy(c);

  return 1;
}
