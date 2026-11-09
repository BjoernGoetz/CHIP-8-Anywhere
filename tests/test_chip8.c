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

static void test_step_reports_unknown_opcode(void) {
  const uint8_t rom[] = {0xE0, 0x00};
  chip8_load_rom(c, rom, sizeof rom);

  struct chip8_step_result r = chip8_step(c);
  TEST_ASSERT_EQUAL(CHIP8_STEP_UNKNOWN_OPCODE, r.status);
  TEST_ASSERT_EQUAL_HEX16(0xE000, r.opcode);
}

static void test_00E0_clears_display(void) {
  const uint8_t rom[] = {0x00, 0xE0};
  chip8_load_rom(c, rom, sizeof rom);

  for (uint8_t y = 0; y < 32; y++) {
    for (uint8_t x = 0; x < 64; x++) {
      chip8_write_pixel(c, x, y, true);
    }
  }

  struct chip8_step_result r = chip8_step(c);

  TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);

  for (uint8_t y = 0; y < 32; y++) {
    for (uint8_t x = 0; x < 64; x++) {
      TEST_ASSERT_FALSE(chip8_read_pixel(c, x, y));
    }
  }
}

static void test_step_advances_pc_to_next_instruction(void) {
  const uint8_t rom[] = {0x00, 0xE0};
  chip8_load_rom(c, rom, sizeof rom);

  chip8_step(c);

  TEST_ASSERT_EQUAL_HEX16(0x202, chip8_read_pc(c));
}

static void test_step_executes_instruction_at_pc(void) {
  chip8_write_memory(c, 0x300, 0xE0);
  chip8_write_memory(c, 0x301, 0x00);
  chip8_write_pc(c, 0x300);

  struct chip8_step_result r = chip8_step(c);

  TEST_ASSERT_EQUAL(CHIP8_STEP_UNKNOWN_OPCODE, r.status);
  TEST_ASSERT_EQUAL_HEX16(0xE000, r.opcode);
}

static void test_ANNN_sets_i_to_nnn(void) {
  const struct {
    uint16_t opcode;
    uint16_t expected_i;
  } cases[] = {
      {0xA22A, 0x22A},
      {0xA000, 0x000},
      {0xAFFF, 0xFFF},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    struct chip8 *m = chip8_create();
    const uint8_t rom[] = {(uint8_t)(cases[n].opcode >> 8),
                           (uint8_t)(cases[n].opcode & 0xFF)};
    chip8_load_rom(m, rom, sizeof rom);

    struct chip8_step_result r = chip8_step(m);
    uint16_t actual_i = chip8_read_i(m);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_i, actual_i);
  }
}

static void test_6XNN_sets_vx_to_nn(void) {
  const struct {
    uint16_t opcode;
    uint8_t x;
    uint8_t expected_vx;
  } cases[] = {
      {0x600C, 0x0, 0x0C},
      {0x6A42, 0xA, 0x42},
      {0x6FFF, 0xF, 0xFF},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    struct chip8 *m = chip8_create();
    const uint8_t rom[] = {(uint8_t)(cases[n].opcode >> 8),
                           (uint8_t)(cases[n].opcode & 0xFF)};
    chip8_load_rom(m, rom, sizeof rom);

    struct chip8_step_result r = chip8_step(m);
    uint8_t actual_vx = chip8_read_v(m, cases[n].x);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected_vx, actual_vx);
  }
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_sets_pc_to_program_start);
  RUN_TEST(test_load_rom_copies_rom_to_0x200);
  RUN_TEST(test_step_reports_unknown_opcode);
  RUN_TEST(test_00E0_clears_display);
  RUN_TEST(test_step_advances_pc_to_next_instruction);
  RUN_TEST(test_step_executes_instruction_at_pc);
  RUN_TEST(test_ANNN_sets_i_to_nnn);
  RUN_TEST(test_6XNN_sets_vx_to_nn);
  return UNITY_END();
}
