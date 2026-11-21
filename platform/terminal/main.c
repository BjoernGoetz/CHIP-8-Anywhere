#include "chip8.h"
#include "chip8_platform.h"
#include "emulator.h"
#include <stdio.h>
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }
void chip8_platform_free(void *ptr) { free(ptr); }

static size_t read_rom_file(const char *path, uint8_t *rom, size_t max) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    return 0;
  }
  size_t size = fread(rom, 1, max, f);
  fclose(f);

  return size;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s <rom.ch8>\n", argv[0]);
    return 2;
  }

  uint8_t rom[CHIP8_MAX_ROM_SIZE];
  size_t size = read_rom_file(argv[1], rom, sizeof rom);
  if (size == 0) {
    fprintf(stderr, "cannot open ROM '%s'\n", argv[1]);
    return 1;
  }

  struct chip8 *c = chip8_create();
  chip8_load_rom(c, rom, size);

  emulator_init();

  struct chip8_step_result r;
  do {
    r = emulator_step(c);
  } while (r.status == CHIP8_STEP_OK);

  fprintf(stderr, "unknown opcode: %04X\n", r.opcode);
  emulator_shutdown();
  chip8_destroy(c);

  return 1;
}
