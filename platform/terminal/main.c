#include "chip8.h"
#include "chip8_platform.h"
#include <stdio.h>
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }
void chip8_platform_free(void *ptr) { free(ptr); }

static void draw_display(const struct chip8 *c) {
  for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
      fputs(chip8_read_pixel(c, x, y) ? "██" : "  ", stdout);
    }
    fputc('\n', stdout);
  }
}

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
    draw_display(c);
  } while (r.status == CHIP8_STEP_OK);

  fprintf(stderr, "unknown opcode: %04X\n", r.opcode);
  chip8_destroy(c);

  return 1;
}
