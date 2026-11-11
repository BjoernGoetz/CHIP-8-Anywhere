#include "display.h"

void display_draw(const struct chip8 *c, FILE *out) {
  fputs("\x1b[H", out);
  for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
      fputs(chip8_read_pixel(c, x, y) ? "██" : "  ", out);
    }
    fputc('\n', out);
  }
}
