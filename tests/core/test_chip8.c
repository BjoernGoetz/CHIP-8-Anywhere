#include "chip8_internal.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

void *chip8_platform_alloc(size_t size) { return malloc(size); }
void chip8_platform_free(void *ptr) { free(ptr); }

static struct chip8 *c;

void setUp(void) { c = chip8_create(); }

void tearDown(void) { chip8_destroy(c); }

static void load_instruction(struct chip8 *m, uint16_t opcode) {
  const uint8_t rom[] = {(uint8_t)(opcode >> 8), (uint8_t)(opcode & 0xFF)};
  chip8_load_rom(m, rom, sizeof rom);
}

static void reset_machine(void) {
  chip8_destroy(c);
  c = chip8_create();
}

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
  load_instruction(c, 0xE000);

  struct chip8_step_result r = chip8_step(c);
  TEST_ASSERT_EQUAL(CHIP8_STEP_UNKNOWN_OPCODE, r.status);
  TEST_ASSERT_EQUAL_HEX16(0xE000, r.opcode);
}

static void test_00E0_clears_display(void) {
  load_instruction(c, 0x00E0);

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
  load_instruction(c, 0x00E0);

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
    reset_machine();
    load_instruction(c, cases[n].opcode);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
  }
}

static void test_2NNN_jumps_to_nnn(void) {
  const struct {
    uint16_t opcode;
    uint16_t expected_pc;
  } cases[] = {
      {0x222C, 0x22C},
      {0x2ABC, 0xABC},
      {0x2200, 0x200},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
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
    reset_machine();
    load_instruction(c, cases[n].opcode);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_i, c->i);
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
    reset_machine();
    load_instruction(c, cases[n].opcode);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected_vx, c->v[cases[n].x]);
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
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x] = cases[n].vx;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected_vx, c->v[cases[n].x]);
  }
}

static const char *read_display_row(const struct chip8 *m, int x, int y) {
  static char row[9];
  for (int bit = 0; bit < 8; bit++) {
    row[bit] = m->display[y][x + bit] ? '1' : '0';
  }
  return row;
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
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->i = cases[n].i;
    c->memory[cases[n].i] = cases[n].sprite;
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_STRING(cases[n].row,
                             read_display_row(c, cases[n].x, cases[n].y));
  }
}

static void test_DXYN_draws_n_rows(void) {
  load_instruction(c, 0xD013);

  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      c->display[y][x] = false;
    }
  }

  c->i = 0x300;
  c->memory[0x300] = 0xF0; /* 1111 0000 */
  c->memory[0x301] = 0x90; /* 1001 0000 */
  c->memory[0x302] = 0xF0; /* 1111 0000 */
  c->memory[0x303] = 0xFF; /* not part of the sprite: must not be drawn */
  c->v[0] = 4;
  c->v[1] = 2;

  struct chip8_step_result r = chip8_step(c);

  TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
  TEST_ASSERT_EQUAL_STRING("11110000", read_display_row(c, 4, 2));
  TEST_ASSERT_EQUAL_STRING("10010000", read_display_row(c, 4, 3));
  TEST_ASSERT_EQUAL_STRING("11110000", read_display_row(c, 4, 4));
  TEST_ASSERT_EQUAL_STRING("00000000", read_display_row(c, 4, 5));
}

static void test_EXA1_skips_if_key_not_pressed(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t checked_key;
    uint8_t pressed_key;
    uint16_t expected_pc;
  } cases[] = {
      {0xE0A1, 0x0, 0x5, 0xA, 0x204}, /* not pressed */
      {0xE7A1, 0x7, 0xA, 0xA, 0x202}, /* pressed */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->key_pressed, false, sizeof c->key_pressed);
    c->key_pressed[cases[n].pressed_key] = true;
    memset(c->v, 0, sizeof c->v);
    c->v[cases[n].x_reg] = cases[n].checked_key;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
  }
}

static void test_FX65_loads_v0_to_vx_from_memory(void) {
  const uint8_t data[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
  const struct {
    uint16_t opcode;
    uint16_t i;
    uint8_t expected_v[16];
  } cases[] = {
      {0xF065,
       0x300,
       {0x10, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
        0xAA, 0xAA, 0xAA, 0xAA}},
      {0xF365,
       0x5A4,
       {0x10, 0x11, 0x12, 0x13, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
        0xAA, 0xAA, 0xAA, 0xAA}},
      {0xFF65,
       0xFF0,
       {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
        0x1C, 0x1D, 0x1E, 0x1F}},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->i = cases[n].i;
    memcpy(&c->memory[cases[n].i], data, sizeof data);
    memset(c->v, 0xAA, sizeof c->v);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(cases[n].expected_v, c->v, 16);
  }
}

static void test_FX65_increments_i_by_x_plus_1(void) {
  load_instruction(c, 0xF365);
  c->i = 0x300;

  struct chip8_step_result r = chip8_step(c);

  TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
  TEST_ASSERT_EQUAL_HEX16(0x304, c->i);
}

static void test_8XY0_set_vx_to_value_of_vy(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
  } cases[] = {
      {0x8010, 0x0, 0x1, 0x00, 0xFF},
      {0x8540, 0x5, 0x4, 0x20, 0x0C},
      {0x8FE0, 0xF, 0xE, 0xFF, 0x00},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].y, c->v[cases[n].x_reg]);
  }
}

static void test_FX1E_adds_vx_to_i(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t vx;
    uint16_t i;
    uint16_t expected_i;
  } cases[] = {
      {0xF01E, 0x0, 0x50, 0x100, 0x150},
      {0xF61E, 0x6, 0x20, 0x2F0, 0x310},
      {0xFF1E, 0xF, 0xFF, 0x528, 0x627},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].vx;
    c->i = cases[n].i;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_i, c->i);
  }
}

static void test_FX15_sets_delay_timer_to_vx(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t vx;
  } cases[] = {
      {0xF015, 0x0, 0x01},
      {0xFA15, 0xA, 0x80},
      {0xFF15, 0xF, 0xFF},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].vx;
    c->delay_timer = 0xAA;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].vx, c->delay_timer);
  }
}

static void test_FX18_sets_sound_timer_to_vx(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t vx;
  } cases[] = {
      {0xF018, 0x0, 0x05},
      {0xF718, 0x7, 0x3C},
      {0xFF18, 0xF, 0xFF},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].vx;
    c->sound_timer = 0xAA;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].vx, c->sound_timer);
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
  RUN_TEST(test_2NNN_jumps_to_nnn);
  RUN_TEST(test_ANNN_sets_i_to_nnn);
  RUN_TEST(test_6XNN_sets_vx_to_nn);
  RUN_TEST(test_7XNN_adds_nn_to_vx);
  RUN_TEST(test_8XY0_set_vx_to_value_of_vy);
  RUN_TEST(test_DXYN_draws_sprite_row);
  RUN_TEST(test_DXYN_draws_n_rows);
  RUN_TEST(test_EXA1_skips_if_key_not_pressed);
  RUN_TEST(test_FX65_loads_v0_to_vx_from_memory);
  RUN_TEST(test_FX65_increments_i_by_x_plus_1);
  RUN_TEST(test_FX1E_adds_vx_to_i);
  RUN_TEST(test_FX15_sets_delay_timer_to_vx);
  RUN_TEST(test_FX18_sets_sound_timer_to_vx);
  return UNITY_END();
}
