#include "chip8_internal.h"
#include "unity.h"
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }

static struct chip8 *c;

void setUp(void) { c = chip8_create(); }

void tearDown(void) {}

static void test_memory_is_4096_bytes(void) {
  TEST_ASSERT_EQUAL(4096, chip8_memory_size(c));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_memory_is_4096_bytes);
  return UNITY_END();
}
