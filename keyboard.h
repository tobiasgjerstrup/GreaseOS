#pragma once

#include <stdint.h>

enum
{
    KEY_UP = 256,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT
};

int keyboard_has_data(void);
int keyboard_read_key(void);
