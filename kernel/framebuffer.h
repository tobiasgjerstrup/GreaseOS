#pragma once

#include <stdint.h>

struct fb_info
{
    uint64_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t red_pos;
    uint8_t red_size;
    uint8_t green_pos;
    uint8_t green_size;
    uint8_t blue_pos;
    uint8_t blue_size;
};

int fb_init(const void *mb2_info);
int fb_is_available(void);
const struct fb_info *fb_get_info(void);
void fb_clear(uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_demo(void);
