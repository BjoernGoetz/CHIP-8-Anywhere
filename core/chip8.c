#include "chip8.h"
#include "chip8_internal.h"
#include "chip8_platform.h"
#include <stdint.h>

#define PROGRAM_START 0x200
#define DISPLAY_HEIGHT 32
#define DISPLAY_WIDTH 64

struct chip8 *chip8_create(void) {
  struct chip8 *c = chip8_platform_alloc(sizeof(struct chip8));
  c->pc = PROGRAM_START;
  return c;
}

void chip8_destroy(struct chip8 *c) { chip8_platform_free(c); }

void chip8_load_rom(struct chip8 *c, const uint8_t *rom, size_t size) {
  for (size_t i = 0; i < size; i++) {
    c->memory[PROGRAM_START + i] = rom[i];
  }
}

bool chip8_read_pixel(const struct chip8 *c, int x, int y) {
  return c->display[y][x];
}

bool chip8_is_sound_active(const struct chip8 *c) {
  return c->sound_timer != 0;
}

static void execute_00E0_clear_screen(struct chip8 *c, uint16_t opcode) {
  (void)opcode;
  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      c->display[y][x] = false;
    }
  }
}

static void execute_00EE_return(struct chip8 *c, uint16_t opcode) {
  (void)opcode;
  c->pc = c->return_address;
}

static void execute_ANNN_set_i(struct chip8 *c, uint16_t opcode) {
  c->i = opcode & 0x0FFF;
}

static void execute_6XNN_set_vx(struct chip8 *c, uint16_t opcode) {
  c->v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
}

static void execute_DXYN_draw(struct chip8 *c, uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  uint8_t y_reg = (opcode & 0x00F0) >> 4;
  uint8_t n = (opcode & 0x000F);

  uint8_t x_offset = c->v[x_reg];
  uint8_t y_offset = c->v[y_reg];

  for (int y = 0; y < n; y++) {
    uint8_t sprite = c->memory[c->i + y];
    for (int x = 0; x < 8; x++) {
      c->display[y_offset + y][x_offset + x] = sprite >> (7 - x) & 1;
    }
  }
}

static void execute_7XNN_add_to_vx(struct chip8 *c, uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  uint16_t nn = opcode & 0x00FF;
  c->v[x_reg] += nn;
}

static void execute_1NNN_jump(struct chip8 *c, uint16_t opcode) {
  c->pc = opcode & 0x0FFF;
}

static void execute_2NNN_call_subroutine(struct chip8 *c, uint16_t opcode) {
  c->return_address = c->pc;
  c->pc = opcode & 0x0FFF;
}

static void execute_FX65_load_registers(struct chip8 *c, uint16_t opcode) {
  uint8_t x = (opcode & 0x0F00) >> 8;
  for (int i = 0; i < x + 1; i++) {
    c->v[i] = c->memory[c->i];
    c->i += 1;
  }
}

static void execute_8XY0_set_vx_to_value_of_vy(struct chip8 *c,
                                               uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  uint8_t y_reg = (opcode & 0x00F0) >> 4;
  c->v[x_reg] = c->v[y_reg];
}

static void execute_FX1E_add_vx_to_i(struct chip8 *c, uint16_t opcode) {
  uint8_t x = (opcode & 0x0F00) >> 8;
  c->i += c->v[x];
}

static void execute_FX15_set_delay_timer_to_vx(struct chip8 *c,
                                               uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  c->delay_timer = c->v[x_reg];
}

static void execute_FX18_set_sound_timer_to_vx(struct chip8 *c,
                                               uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  c->sound_timer = c->v[x_reg];
}

static void execute_EXA1_skip_if_key_not_pressed(struct chip8 *c,
                                                 uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  uint8_t x = c->v[x_reg];
  if (!c->key_pressed[x]) {
    c->pc += 2;
  }
}

static void execute_FX07_set_vx_to_delay_timer(struct chip8 *c,
                                               uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  c->v[x_reg] = c->delay_timer;
}

static void execute_3XNN_skip_if_vx_equals_nn(struct chip8 *c,
                                              uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  uint8_t nn = opcode & 0x00FF;
  if (c->v[x_reg] == nn) {
    c->pc += 2;
  }
}

static void execute_5XY0_skip_if_vx_equals_vy(struct chip8 *c,
                                              uint16_t opcode) {
  uint8_t x_reg = (opcode & 0x0F00) >> 8;
  uint8_t y_reg = (opcode & 0x00F0) >> 4;
  if (c->v[x_reg] == c->v[y_reg]) {
    c->pc += 2;
  }
}

struct instruction {
  uint16_t mask;
  uint16_t pattern;
  void (*execute)(struct chip8 *c, uint16_t opcode);
};

static const struct instruction instructions[] = {
    {0xFFFF, 0x00E0, execute_00E0_clear_screen},
    {0xFFFF, 0x00EE, execute_00EE_return},
    {0xF000, 0x1000, execute_1NNN_jump},
    {0xF000, 0x2000, execute_2NNN_call_subroutine},
    {0xF000, 0x3000, execute_3XNN_skip_if_vx_equals_nn},
    {0xF00F, 0x5000, execute_5XY0_skip_if_vx_equals_vy},
    {0xF000, 0x6000, execute_6XNN_set_vx},
    {0xF000, 0x7000, execute_7XNN_add_to_vx},
    {0xF00F, 0x8000, execute_8XY0_set_vx_to_value_of_vy},
    {0xF000, 0xA000, execute_ANNN_set_i},
    {0xF000, 0xD000, execute_DXYN_draw},
    {0xF0FF, 0xE0A1, execute_EXA1_skip_if_key_not_pressed},
    {0xF0FF, 0xF007, execute_FX07_set_vx_to_delay_timer},
    {0xF0FF, 0xF015, execute_FX15_set_delay_timer_to_vx},
    {0xF0FF, 0xF018, execute_FX18_set_sound_timer_to_vx},
    {0xF0FF, 0xF01E, execute_FX1E_add_vx_to_i},
    {0xF0FF, 0xF065, execute_FX65_load_registers},
};

struct chip8_step_result chip8_step(struct chip8 *c) {
  uint16_t opcode = (uint16_t)(c->memory[c->pc] << 8 | c->memory[c->pc + 1]);
  c->pc += 2;

  for (size_t i = 0; i < sizeof instructions / sizeof instructions[0]; i++) {
    if ((opcode & instructions[i].mask) == instructions[i].pattern) {
      instructions[i].execute(c, opcode);
      return (struct chip8_step_result){CHIP8_STEP_OK, 0};
    }
  }

  return (struct chip8_step_result){CHIP8_STEP_UNKNOWN_OPCODE, opcode};
}

static uint8_t count_down(uint8_t timer, uint32_t ticks) {
  return ticks >= timer ? 0 : (uint8_t)(timer - ticks);
}

void chip8_update_timers(struct chip8 *c, uint32_t elapsed_microseconds) {
  uint32_t total = c->timer_remainder + elapsed_microseconds * 60;
  uint32_t ticks = total / 1000000;
  c->timer_remainder = total % 1000000;
  c->delay_timer = count_down(c->delay_timer, ticks);
  c->sound_timer = count_down(c->sound_timer, ticks);
}
