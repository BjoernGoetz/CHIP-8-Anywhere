#include "chip8_internal.h"
#include "unity.h"
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }

static struct chip8 *c;

void setUp(void) { c = chip8_create(); }

void tearDown(void) {}

static void test_create_sets_pc_to_program_start(void) {
  TEST_ASSERT_EQUAL_HEX16(0x200, chip8_read_pc(c));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_sets_pc_to_program_start);
  return UNITY_END();
}
