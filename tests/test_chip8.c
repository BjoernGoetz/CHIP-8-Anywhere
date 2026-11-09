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

static void test_load_rom_copies_rom_to_0x200(void) {
  const uint8_t rom[] = {0x12, 0x34, 0x56};
  chip8_load_rom(c, rom, sizeof rom);

  uint8_t loaded[sizeof rom];
  for (size_t i = 0; i < sizeof rom; i++) {
    loaded[i] = chip8_read_memory(c, (uint16_t)(0x200 + i));
  }
  TEST_ASSERT_EQUAL_HEX8_ARRAY(rom, loaded, sizeof rom);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_sets_pc_to_program_start);
  RUN_TEST(test_load_rom_copies_rom_to_0x200);
  return UNITY_END();
}
