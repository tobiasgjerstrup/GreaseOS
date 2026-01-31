#pragma once

#include <stdint.h>

typedef struct
{
    int16_t x;
    int16_t y;
    uint8_t buttons;
} mouse_state_t;

void mouse_init(void);
int mouse_has_data(void);
mouse_state_t mouse_read(void);
