#include "chip8_internal.h"
#include "unity.h"
#include <stdlib.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }
void chip8_platform_free(void *ptr) { free(ptr); }

static struct chip8 *c;

void setUp(void) { c = chip8_create(); }

void tearDown(void) { chip8_destroy(c); }

static void test_create_sets_pc_to_program_start(void) {
  TEST_ASSERT_EQUAL_HEX16(0x200, c->pc);
}

static void test_load_rom_copies_rom_to_0x200(void) {
  const uint8_t rom[] = {0x12, 0x34, 0x56};
  chip8_load_rom(c, rom, sizeof rom);

  uint8_t loaded[sizeof rom];
  for (size_t i = 0; i < sizeof rom; i++) {
    loaded[i] = c->memory[0x200 + i];
  }
  TEST_ASSERT_EQUAL_HEX8_ARRAY(rom, loaded, sizeof rom);
}

static void test_read_pixel_reads_display(void) {
  const struct {
    int x, y;
    bool on;
  } cases[] = {
      {10, 5, true},
      {63, 31, true},
      {20, 7, false},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    for (int y = 0; y < 32; y++) {
      for (int x = 0; x < 64; x++) {
        c->display[y][x] = false;
      }
    }
    c->display[cases[n].y][cases[n].x] = cases[n].on;

    TEST_ASSERT_EQUAL(cases[n].on, chip8_read_pixel(c, cases[n].x, cases[n].y));
  }
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
      c->display[y][x] = true;
    }
  }

  struct chip8_step_result r = chip8_step(c);

  TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);

  for (uint8_t y = 0; y < 32; y++) {
    for (uint8_t x = 0; x < 64; x++) {
      TEST_ASSERT_FALSE(c->display[y][x]);
    }
  }
}

static void test_step_advances_pc_to_next_instruction(void) {
  const uint8_t rom[] = {0x00, 0xE0};
  chip8_load_rom(c, rom, sizeof rom);

  chip8_step(c);

  TEST_ASSERT_EQUAL_HEX16(0x202, c->pc);
}

static void test_step_executes_instruction_at_pc(void) {
  c->memory[0x300] = 0xE0;
  c->memory[0x301] = 0x00;
  c->pc = 0x300;

  struct chip8_step_result r = chip8_step(c);

  TEST_ASSERT_EQUAL(CHIP8_STEP_UNKNOWN_OPCODE, r.status);
  TEST_ASSERT_EQUAL_HEX16(0xE000, r.opcode);
}

static void test_1NNN_jumps_to_nnn(void) {
  const struct {
    uint16_t opcode;
    uint16_t expected_pc;
  } cases[] = {
      {0x1228, 0x228},
      {0x1ABC, 0xABC},
      {0x1200, 0x200},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    struct chip8 *m = chip8_create();
    const uint8_t rom[] = {(uint8_t)(cases[n].opcode >> 8),
                           (uint8_t)(cases[n].opcode & 0xFF)};
    chip8_load_rom(m, rom, sizeof rom);

    struct chip8_step_result r = chip8_step(m);
    uint16_t actual_pc = m->pc;
    chip8_destroy(m);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, actual_pc);
  }
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
    uint16_t actual_i = m->i;
    chip8_destroy(m);

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
    uint8_t actual_vx = m->v[cases[n].x];
    chip8_destroy(m);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected_vx, actual_vx);
  }
}

static void test_7XNN_adds_nn_to_vx(void) {
  const struct {
    uint16_t opcode;
    uint8_t x;
    uint8_t vx;
    uint8_t expected_vx;
  } cases[] = {
      {0x7009, 0x0, 0x10, 0x19},
      {0x7A42, 0xA, 0x05, 0x47},
      {0x7E01, 0xE, 0x80, 0x81},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    struct chip8 *m = chip8_create();
    const uint8_t rom[] = {(uint8_t)(cases[n].opcode >> 8),
                           (uint8_t)(cases[n].opcode & 0xFF)};
    chip8_load_rom(m, rom, sizeof rom);
    m->v[cases[n].x] = cases[n].vx;

    struct chip8_step_result r = chip8_step(m);
    uint8_t actual_vx = m->v[cases[n].x];
    chip8_destroy(m);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected_vx, actual_vx);
  }
}

static void test_DXYN_draws_sprite_row(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
    uint16_t i;
    uint8_t sprite;
    const char *row;
  } cases[] = {
      {0xD011, 0x0, 0x1, 0, 0, 0x300, 0x80, "10000000"},
      {0xD231, 0x2, 0x3, 10, 5, 0x5A4, 0xF0, "11110000"},
      {0xDAB1, 0xA, 0xB, 56, 31, 0xFFF, 0xA5, "10100101"},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    struct chip8 *m = chip8_create();
    const uint8_t rom[] = {(uint8_t)(cases[n].opcode >> 8),
                           (uint8_t)(cases[n].opcode & 0xFF)};
    chip8_load_rom(m, rom, sizeof rom);
    m->i = cases[n].i;
    m->memory[cases[n].i] = cases[n].sprite;
    m->v[cases[n].x_reg] = cases[n].x;
    m->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(m);

    char actual[9] = {0};
    for (int bit = 0; bit < 8; bit++) {
      actual[bit] = m->display[cases[n].y][cases[n].x + bit] ? '1' : '0';
    }
    chip8_destroy(m);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_STRING(cases[n].row, actual);
  }
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_sets_pc_to_program_start);
  RUN_TEST(test_load_rom_copies_rom_to_0x200);
  RUN_TEST(test_read_pixel_reads_display);
  RUN_TEST(test_step_reports_unknown_opcode);
  RUN_TEST(test_00E0_clears_display);
  RUN_TEST(test_step_advances_pc_to_next_instruction);
  RUN_TEST(test_step_executes_instruction_at_pc);
  RUN_TEST(test_1NNN_jumps_to_nnn);
  RUN_TEST(test_ANNN_sets_i_to_nnn);
  RUN_TEST(test_6XNN_sets_vx_to_nn);
  RUN_TEST(test_7XNN_adds_nn_to_vx);
  RUN_TEST(test_DXYN_draws_sprite_row);
  return UNITY_END();
}
