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

static uint32_t microseconds_for_ticks(uint32_t ticks) {
  return (ticks * 1000000 + 59) / 60;
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

static void test_is_sound_active_only_while_sound_timer_is_nonzero(void) {
  const struct {
    uint8_t sound_timer;
    bool expected;
  } cases[] = {
      {0, false},
      {1, true},
      {255, true},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    c->sound_timer = cases[n].sound_timer;

    TEST_ASSERT_EQUAL(cases[n].expected, chip8_is_sound_active(c));
  }
}

static void test_set_key_presses_and_releases_key(void) {
  const struct {
    enum chip8_key key;
    bool pressed;
  } cases[] = {
      {CHIP8_KEY_0, true},  {CHIP8_KEY_5, true},  {CHIP8_KEY_F, true},
      {CHIP8_KEY_0, false}, {CHIP8_KEY_5, false}, {CHIP8_KEY_F, false},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    memset(c->key_pressed, !cases[n].pressed, sizeof c->key_pressed);

    chip8_set_key(c, cases[n].key, cases[n].pressed);

    TEST_ASSERT_EQUAL(cases[n].pressed, c->key_pressed[cases[n].key]);
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

static void test_00EE_returns_from_subroutine(void) {
  const struct {
    uint8_t rom[10];
    int steps;
    uint16_t expected_pc;
  } cases[] = {
      /* 200: call 204, 202: -, 204: return */
      {{0x22, 0x04, 0x00, 0x00, 0x00, 0xEE}, 2, 0x202},
      /* 200: jump 204, 202: -, 204: call 208, 206: -, 208: return */
      {{0x12, 0x04, 0x00, 0x00, 0x22, 0x08, 0x00, 0x00, 0x00, 0xEE}, 3, 0x206},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    chip8_load_rom(c, cases[n].rom, sizeof cases[n].rom);

    for (int s = 0; s < cases[n].steps; s++) {
      TEST_ASSERT_EQUAL(CHIP8_STEP_OK, chip8_step(c).status);
    }

    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
  }
}

static void test_00EE_returns_from_nested_subroutines_in_reverse_order(void) {
  const uint8_t rom[] = {0x22, 0x04, 0x00, 0x00, 0x22, 0x08, 0x00,
                         0xEE, 0x22, 0x0C, 0x00, 0xEE, 0x00, 0xEE};
  const struct {
    int steps;
    uint16_t expected_pc;
  } cases[] = {
      {4, 0x20A},
      {5, 0x206},
      {6, 0x202},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    chip8_load_rom(c, rom, sizeof rom);

    for (int s = 0; s < cases[n].steps; s++) {
      TEST_ASSERT_EQUAL(CHIP8_STEP_OK, chip8_step(c).status);
    }

    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
  }
}

static void test_2NNN_nests_calls_16_levels_deep(void) {
  uint8_t rom[66];
  for (int level = 0; level < 16; level++) {
    uint16_t call = (uint16_t)(0x2000 | (0x200 + 4 * level + 4));
    rom[4 * level] = call >> 8;
    rom[4 * level + 1] = call & 0xFF;
    rom[4 * level + 2] = 0x00;
    rom[4 * level + 3] = 0xEE;
  }
  rom[64] = 0x00;
  rom[65] = 0xEE;
  chip8_load_rom(c, rom, sizeof rom);

  for (int s = 0; s < 32; s++) {
    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, chip8_step(c).status);
  }

  TEST_ASSERT_EQUAL_HEX16(0x202, c->pc);
}

static void test_3XNN_skips_if_vx_equals_nn(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t vx;
    uint16_t expected_pc;
  } cases[] = {
      {0x3742, 0x7, 0x42, 0x204}, /* equal */
      {0x3A0A, 0xA, 0x1A, 0x202}, /* not equal */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->v, 0, sizeof c->v);
    c->v[cases[n].x_reg] = cases[n].vx;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
  }
}

static void test_4XNN_skips_if_vx_not_equals_nn(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t vx;
    uint16_t expected_pc;
  } cases[] = {
      {0x452A, 0x5, 0x2A, 0x202}, /* equal */
      {0x4A0A, 0xA, 0x1A, 0x204}, /* not equal */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->v, 0, sizeof c->v);
    c->v[cases[n].x_reg] = cases[n].vx;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
  }
}

static void test_5XY0_skips_if_vx_equals_vy(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t vx, vy;
    uint16_t expected_pc;
  } cases[] = {
      {0x5230, 0x2, 0x3, 0x42, 0x42, 0x204}, /* equal */
      {0x5230, 0x2, 0x3, 0x42, 0x43, 0x202}, /* not equal */
      {0x5AF0, 0xA, 0xF, 0xFF, 0x00, 0x202}, /* not equal, other registers */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->v, 0, sizeof c->v);
    c->v[cases[n].x_reg] = cases[n].vx;
    c->v[cases[n].y_reg] = cases[n].vy;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(cases[n].expected_pc, c->pc);
  }
}

static void test_9XY0_skips_if_vx_not_equals_vy(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t vx, vy;
    uint16_t expected_pc;
  } cases[] = {
      {0x9560, 0x5, 0x6, 0x42, 0x42, 0x202}, /* equal */
      {0x9560, 0x5, 0x6, 0x42, 0x43, 0x204}, /* not equal */
      {0x9AF0, 0xA, 0xF, 0xFF, 0x00, 0x204}, /* not equal, other registers */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->v, 0, sizeof c->v);
    c->v[cases[n].x_reg] = cases[n].vx;
    c->v[cases[n].y_reg] = cases[n].vy;

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

static void test_DXYN_xors_sprite_onto_display(void) {
  const struct {
    uint8_t first_sprite;
    uint8_t second_sprite;
    const char *row;
  } cases[] = {
      {0xFF, 0xF0, "00001111"}, /* 1 bits turn lit pixels off */
      {0x0F, 0xF0, "11111111"}, /* 0 bits leave pixels unchanged */
      {0xA5, 0xA5, "00000000"}, /* drawing the same sprite twice clears it */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, 0xD011);
    memset(c->display, false, sizeof c->display);
    c->i = 0x300;
    c->v[0] = 8;
    c->v[1] = 3;
    c->memory[0x300] = cases[n].first_sprite;
    chip8_step(c);
    c->pc = 0x200;
    c->memory[0x300] = cases[n].second_sprite;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_STRING(cases[n].row, read_display_row(c, 8, 3));
  }
}

static void test_EX9E_skips_if_key_pressed(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t checked_key;
    uint8_t pressed_key;
    uint16_t expected_pc;
  } cases[] = {
      {0xE49E, 0x4, 0x5, 0x5, 0x204}, /* pressed */
      {0xE09E, 0x0, 0x5, 0xA, 0x202}, /* not pressed */
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

static void test_FX55_stores_v0_to_vx_in_memory(void) {
  const uint8_t data[16] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
  const struct {
    uint16_t opcode;
    uint16_t i;
    uint8_t expected_memory[16];
  } cases[] = {
      {0xF055,
       0x300,
       {0x10, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
        0xAA, 0xAA, 0xAA, 0xAA}},
      {0xF355,
       0x5A4,
       {0x10, 0x11, 0x12, 0x13, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
        0xAA, 0xAA, 0xAA, 0xAA}},
      {0xFF55,
       0xFF0,
       {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
        0x1C, 0x1D, 0x1E, 0x1F}},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->i = cases[n].i;
    memcpy(c->v, data, sizeof data);
    memset(&c->memory[cases[n].i], 0xAA, sizeof data);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(cases[n].expected_memory,
                                 &c->memory[cases[n].i], 16);
  }
}

static void test_FX55_increments_i_by_x_plus_1(void) {
  load_instruction(c, 0xF355);
  c->i = 0x300;

  struct chip8_step_result r = chip8_step(c);

  TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
  TEST_ASSERT_EQUAL_HEX16(0x304, c->i);
}

static void test_FX33_stores_vx_as_decimal_digits(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t vx;
    uint16_t i;
    uint8_t expected_memory[3];
  } cases[] = {
      {0xF633, 0x6, 156, 0x300, {1, 5, 6}},
      {0xF033, 0x0, 7, 0x5A4, {0, 0, 7}}, /* leading zeros */
      {0xFE33, 0xE, 255, 0xFFD, {2, 5, 5}},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->i = cases[n].i;
    c->v[cases[n].x_reg] = cases[n].vx;
    memset(&c->memory[cases[n].i], 0xAA, 3);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cases[n].expected_memory,
                                  &c->memory[cases[n].i], 3);
  }
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

static void test_8XY1_sets_vx_to_vx_or_vy(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
    uint8_t expected;
  } cases[] = {
      {0x8761, 0x7, 0x6, 0xCA, 0x56, 0xDE},
      {0x8011, 0x0, 0x1, 0xF0, 0x0F, 0xFF},
      {0x8AB1, 0xA, 0xB, 0x81, 0x81, 0x81},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].x_reg]);
  }
}

static void test_8XY2_sets_vx_to_vx_and_vy(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
    uint8_t expected;
  } cases[] = {
      {0x8762, 0x7, 0x6, 0xCA, 0x56, 0x42},
      {0x8012, 0x0, 0x1, 0xF0, 0x0F, 0x00},
      {0x8AB2, 0xA, 0xB, 0x81, 0x81, 0x81},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].x_reg]);
  }
}

static void test_8XY3_sets_vx_to_vx_xor_vy(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
    uint8_t expected;
  } cases[] = {
      {0x8763, 0x7, 0x6, 0xCA, 0x56, 0x9C},
      {0x8013, 0x0, 0x1, 0xF0, 0x0F, 0xFF},
      {0x8AB3, 0xA, 0xB, 0x81, 0x81, 0x00},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].x_reg]);
  }
}

static void test_8XY4_adds_vy_to_vx(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
    uint8_t expected;
  } cases[] = {
      {0x8764, 0x7, 0x6, 0x10, 0x20, 0x30},
      {0x8764, 0x7, 0x6, 0xCA, 0x56, 0x20}, /* wraps around past 0xFF */
      {0x8014, 0x0, 0x1, 0xFF, 0x01, 0x00},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].x_reg]);
  }
}

static void test_8XY5_subtracts_vy_from_vx(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
    uint8_t expected;
  } cases[] = {
      {0x8765, 0x7, 0x6, 0x30, 0x10, 0x20},
      {0x8765, 0x7, 0x6, 0x10, 0x30, 0xE0}, /* wraps around below 0 */
      {0x8015, 0x0, 0x1, 0x42, 0x42, 0x00},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].x_reg]);
  }
}

static void test_8XY6_shifts_right_by_one(void) {
  const struct {
    uint16_t opcode;
    uint8_t reg;
    uint8_t value;
    uint8_t expected;
  } cases[] = {
      {0x8666, 0x6, 0xCB, 0x65},
      {0x8116, 0x1, 0x80, 0x40},
      {0x8006, 0x0, 0x01, 0x00}, /* lowest bit falls off */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].reg] = cases[n].value;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].reg]);
  }
}

static void test_8XY7_sets_vx_to_vy_minus_vx(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg, y_reg;
    uint8_t x, y;
    uint8_t expected;
  } cases[] = {
      {0x8767, 0x7, 0x6, 0x10, 0x30, 0x20},
      {0x8767, 0x7, 0x6, 0x30, 0x10, 0xE0}, /* wraps around below 0 */
      {0x8017, 0x0, 0x1, 0x42, 0x42, 0x00},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].x_reg] = cases[n].x;
    c->v[cases[n].y_reg] = cases[n].y;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].x_reg]);
  }
}

static void test_8XYE_shifts_left_by_one(void) {
  const struct {
    uint16_t opcode;
    uint8_t reg;
    uint8_t value;
    uint8_t expected;
  } cases[] = {
      {0x866E, 0x6, 0xCB, 0x96},
      {0x811E, 0x1, 0x01, 0x02},
      {0x800E, 0x0, 0x80, 0x00}, /* highest bit falls off */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->v[cases[n].reg] = cases[n].value;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].expected, c->v[cases[n].reg]);
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

static void test_FX07_sets_vx_to_delay_timer(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t delay_timer;
  } cases[] = {
      {0xF007, 0x0, 0x3C},
      {0xF907, 0x9, 0x01},
      {0xFF07, 0xF, 0xFF},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    c->delay_timer = cases[n].delay_timer;
    c->v[cases[n].x_reg] = 0xAA;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX8(cases[n].delay_timer, c->v[cases[n].x_reg]);
  }
}

static void test_FX0A_waits_while_no_key_is_pressed(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
  } cases[] = {
      {0xF00A, 0x0},
      {0xF30A, 0x3},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->key_pressed, false, sizeof c->key_pressed);
    c->v[cases[n].x_reg] = 0xAA;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(0x200, c->pc);
    TEST_ASSERT_EQUAL_HEX8(0xAA, c->v[cases[n].x_reg]);
  }
}

static void test_FX0A_waits_while_key_is_held(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t key;
  } cases[] = {
      {0xF00A, 0x0, 0x5},
      {0xF30A, 0x3, 0xE},
      {0xF70A, 0x7, 0xF},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->key_pressed, false, sizeof c->key_pressed);
    c->key_pressed[cases[n].key] = true;
    c->v[cases[n].x_reg] = 0xAA;
    chip8_step(c);

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(0x200, c->pc);
    TEST_ASSERT_EQUAL_HEX8(0xAA, c->v[cases[n].x_reg]);
  }
}

static void test_FX0A_stores_key_when_released(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t key;
  } cases[] = {
      {0xF00A, 0x0, 0x5},
      {0xF30A, 0x3, 0xE},
      {0xF70A, 0x7, 0xF},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->key_pressed, false, sizeof c->key_pressed);
    c->key_pressed[cases[n].key] = true;
    chip8_step(c);
    c->key_pressed[cases[n].key] = false;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(0x202, c->pc);
    TEST_ASSERT_EQUAL_HEX8(cases[n].key, c->v[cases[n].x_reg]);
  }
}

static void test_FX0A_waits_again_after_release(void) {
  const struct {
    uint16_t opcode;
    uint8_t x_reg;
    uint8_t key;
  } cases[] = {
      {0xF00A, 0x0, 0x5},
      {0xF30A, 0x3, 0xE},
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    load_instruction(c, cases[n].opcode);
    memset(c->key_pressed, false, sizeof c->key_pressed);
    c->key_pressed[cases[n].key] = true;
    chip8_step(c);
    c->key_pressed[cases[n].key] = false;
    chip8_step(c);
    c->pc = 0x200;

    struct chip8_step_result r = chip8_step(c);

    TEST_ASSERT_EQUAL(CHIP8_STEP_OK, r.status);
    TEST_ASSERT_EQUAL_HEX16(0x200, c->pc);
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

static void test_update_timers_ticks_at_60_hz(void) {
  const struct {
    uint8_t timer;
    uint32_t elapsed_microseconds;
    uint8_t expected_timer;
  } cases[] = {
      {10, 0, 10},     /* no time passed */
      {10, 16666, 10}, /* just under 1/60 s */
      {10, 16667, 9},  /* one tick */
      {10, 50000, 7},  /* exactly three ticks */
  };

  for (size_t n = 0; n < sizeof cases / sizeof cases[0]; n++) {
    reset_machine();
    c->delay_timer = cases[n].timer;
    c->sound_timer = cases[n].timer;

    chip8_update_timers(c, cases[n].elapsed_microseconds);

    TEST_ASSERT_EQUAL_UINT8(cases[n].expected_timer, c->delay_timer);
    TEST_ASSERT_EQUAL_UINT8(cases[n].expected_timer, c->sound_timer);
  }
}

static void test_update_timers_stops_at_zero(void) {
  c->delay_timer = 1;
  c->sound_timer = 1;

  chip8_update_timers(c, microseconds_for_ticks(255));

  TEST_ASSERT_EQUAL_UINT8(0, c->delay_timer);
  TEST_ASSERT_EQUAL_UINT8(0, c->sound_timer);
}

static void test_update_timers_carries_remainder(void) {
  c->delay_timer = 10;
  c->sound_timer = 10;

  chip8_update_timers(c, 8333);
  chip8_update_timers(c, 8334); /* together exactly 16 667 µs */

  TEST_ASSERT_EQUAL_UINT8(9, c->delay_timer);
  TEST_ASSERT_EQUAL_UINT8(9, c->sound_timer);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_sets_pc_to_program_start);
  RUN_TEST(test_load_rom_copies_rom_to_0x200);
  RUN_TEST(test_read_pixel_reads_display);
  RUN_TEST(test_is_sound_active_only_while_sound_timer_is_nonzero);
  RUN_TEST(test_set_key_presses_and_releases_key);
  RUN_TEST(test_step_reports_unknown_opcode);
  RUN_TEST(test_00E0_clears_display);
  RUN_TEST(test_step_advances_pc_to_next_instruction);
  RUN_TEST(test_step_executes_instruction_at_pc);
  RUN_TEST(test_1NNN_jumps_to_nnn);
  RUN_TEST(test_2NNN_jumps_to_nnn);
  RUN_TEST(test_00EE_returns_from_subroutine);
  RUN_TEST(test_00EE_returns_from_nested_subroutines_in_reverse_order);
  RUN_TEST(test_2NNN_nests_calls_16_levels_deep);
  RUN_TEST(test_3XNN_skips_if_vx_equals_nn);
  RUN_TEST(test_4XNN_skips_if_vx_not_equals_nn);
  RUN_TEST(test_5XY0_skips_if_vx_equals_vy);
  RUN_TEST(test_9XY0_skips_if_vx_not_equals_vy);
  RUN_TEST(test_ANNN_sets_i_to_nnn);
  RUN_TEST(test_6XNN_sets_vx_to_nn);
  RUN_TEST(test_7XNN_adds_nn_to_vx);
  RUN_TEST(test_8XY0_set_vx_to_value_of_vy);
  RUN_TEST(test_8XY1_sets_vx_to_vx_or_vy);
  RUN_TEST(test_8XY2_sets_vx_to_vx_and_vy);
  RUN_TEST(test_8XY3_sets_vx_to_vx_xor_vy);
  RUN_TEST(test_8XY4_adds_vy_to_vx);
  RUN_TEST(test_8XY5_subtracts_vy_from_vx);
  RUN_TEST(test_8XY6_shifts_right_by_one);
  RUN_TEST(test_8XY7_sets_vx_to_vy_minus_vx);
  RUN_TEST(test_8XYE_shifts_left_by_one);
  RUN_TEST(test_DXYN_draws_sprite_row);
  RUN_TEST(test_DXYN_draws_n_rows);
  RUN_TEST(test_DXYN_xors_sprite_onto_display);
  RUN_TEST(test_EX9E_skips_if_key_pressed);
  RUN_TEST(test_EXA1_skips_if_key_not_pressed);
  RUN_TEST(test_FX65_loads_v0_to_vx_from_memory);
  RUN_TEST(test_FX65_increments_i_by_x_plus_1);
  RUN_TEST(test_FX55_stores_v0_to_vx_in_memory);
  RUN_TEST(test_FX55_increments_i_by_x_plus_1);
  RUN_TEST(test_FX33_stores_vx_as_decimal_digits);
  RUN_TEST(test_FX1E_adds_vx_to_i);
  RUN_TEST(test_FX07_sets_vx_to_delay_timer);
  RUN_TEST(test_FX0A_waits_while_no_key_is_pressed);
  RUN_TEST(test_FX0A_waits_while_key_is_held);
  RUN_TEST(test_FX0A_stores_key_when_released);
  RUN_TEST(test_FX0A_waits_again_after_release);
  RUN_TEST(test_FX15_sets_delay_timer_to_vx);
  RUN_TEST(test_FX18_sets_sound_timer_to_vx);
  RUN_TEST(test_update_timers_ticks_at_60_hz);
  RUN_TEST(test_update_timers_stops_at_zero);
  RUN_TEST(test_update_timers_carries_remainder);
  return UNITY_END();
}
