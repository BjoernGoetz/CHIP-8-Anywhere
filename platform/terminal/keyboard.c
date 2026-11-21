#include "keyboard.h"
#include <fcntl.h>
#include <stdbool.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define RELEASE_AFTER_MICROSECONDS 100000

static uint64_t last_seen[16];

static uint64_t now_microseconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

void keyboard_init(void) {
  struct termios t;
  if (tcgetattr(STDIN_FILENO, &t) == 0) {
    t.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
    t.c_cc[VMIN] = 0; /* read returns at once, even with no input */
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
  } else {
    /* A pipe or file, as in the smoke tests: don't block on read. */
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
  }
}

static const struct {
  char character;
  enum chip8_key key;
} keymap[] = {
    {'1', CHIP8_KEY_1}, {'2', CHIP8_KEY_2}, {'3', CHIP8_KEY_3},
    {'4', CHIP8_KEY_C}, {'q', CHIP8_KEY_4}, {'w', CHIP8_KEY_5},
    {'e', CHIP8_KEY_6}, {'r', CHIP8_KEY_D}, {'a', CHIP8_KEY_7},
    {'s', CHIP8_KEY_8}, {'d', CHIP8_KEY_9}, {'f', CHIP8_KEY_E},
    {'z', CHIP8_KEY_A}, {'x', CHIP8_KEY_0}, {'c', CHIP8_KEY_B},
    {'v', CHIP8_KEY_F},
};

static int key_for_char(char character) {
  for (size_t i = 0; i < sizeof keymap / sizeof keymap[0]; i++) {
    if (keymap[i].character == character) {
      return keymap[i].key;
    }
  }

  return -1;
}

void keyboard_update(struct chip8 *c) {
  uint64_t now = now_microseconds();
  char character;

  while (read(STDIN_FILENO, &character, 1) == 1) {
    int key = key_for_char(character);
    if (key >= 0) {
      last_seen[key] = now;
    }
  }

  for (int key = 0; key < 16; key++) {
    chip8_set_key(c, key, now - last_seen[key] < RELEASE_AFTER_MICROSECONDS);
  }
}
