#include "console.h"

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static const uint16_t VGA_WIDTH = 80;
static const uint16_t VGA_HEIGHT = 25;
static uint16_t vga_row = 0;
static uint16_t vga_col = 0;
static uint8_t vga_color = 0x0F;

static void console_scroll(void)
{
    for (uint16_t y = 1; y < VGA_HEIGHT; ++y)
    {
        for (uint16_t x = 0; x < VGA_WIDTH; ++x)
        {
            VGA[(y - 1) * VGA_WIDTH + x] = VGA[y * VGA_WIDTH + x];
        }
    }
    for (uint16_t x = 0; x < VGA_WIDTH; ++x)
    {
        VGA[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (uint16_t)vga_color << 8 | ' ';
    }
    if (vga_row > 0)
    {
        vga_row--;
    }
}

void console_clear(void)
{
    for (uint16_t y = 0; y < VGA_HEIGHT; ++y)
    {
        for (uint16_t x = 0; x < VGA_WIDTH; ++x)
        {
            VGA[y * VGA_WIDTH + x] = (uint16_t)vga_color << 8 | ' ';
        }
    }
    vga_row = 0;
    vga_col = 0;
}

void console_putc(char c)
{
    if (c == '\n')
    {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT)
        {
            console_scroll();
        }
        return;
    }

    VGA[vga_row * VGA_WIDTH + vga_col] = (uint16_t)vga_color << 8 | (uint8_t)c;
    vga_col++;
    if (vga_col >= VGA_WIDTH)
    {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT)
        {
            console_scroll();
        }
    }
}

void console_write(const char* msg)
{
    for (size_t i = 0; msg[i] != '\0'; ++i)
    {
        console_putc(msg[i]);
    }
}

void console_backspace(void)
{
    if (vga_col == 0 && vga_row == 0)
    {
        return;
    }

    if (vga_col == 0)
    {
        vga_row--;
        vga_col = VGA_WIDTH - 1;
    }
    else
    {
        vga_col--;
    }

    VGA[vga_row * VGA_WIDTH + vga_col] = (uint16_t)vga_color << 8 | ' ';
}
