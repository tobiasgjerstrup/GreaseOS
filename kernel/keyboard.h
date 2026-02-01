#pragma once

#include <stdint.h>

enum
{
    KEY_UP = 256,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_CTRL_A = 1,
    KEY_CTRL_C = 3,
    KEY_CTRL_X = 24,
    KEY_CTRL_V = 22,
    KEY_CTRL_Z = 26
};

int keyboard_has_data(void);
int keyboard_read_key(void);
void keyboard_init(void);
uint8_t keyboard_last_scancode(void);
uint8_t keyboard_last_status(void);
