#include "chip8_internal.h"
#include "display.h"
#include "unity.h"
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }
void chip8_platform_free(void *ptr) { free(ptr); }

static struct chip8 *c;

void setUp(void) {
  c = chip8_create();
  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      c->display[y][x] = false;
    }
  }
}

void tearDown(void) { chip8_destroy(c); }

static void read_all(FILE *f, char *buf, size_t size) {
  rewind(f);
  size_t len = fread(buf, 1, size - 1, f);
  buf[len] = '\0';
  fclose(f);
}

static void test_draw_outputs_whole_screen(void) {
  for (int row = 0; row < 4; row++) {
    for (int i = 0; i <= row; i++) {
      c->display[14 + row][30 + i] = true;
    }
  }

  FILE *f = tmpfile();
  display_draw(c, f);
  static char actual[16384];
  read_all(f, actual, sizeof actual);

  static char expected[16384];
  read_all(fopen("tests/platform/terminal/frame.txt", "rb"), expected,
           sizeof expected);

  TEST_ASSERT_EQUAL_STRING(expected, actual);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_draw_outputs_whole_screen);
  return UNITY_END();
}
