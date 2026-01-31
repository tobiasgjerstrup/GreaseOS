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

void console_putc_at(uint16_t row, uint16_t col, char c)
{
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH)
    {
        return;
    }
    VGA[row * VGA_WIDTH + col] = (uint16_t)vga_color << 8 | (uint8_t)c;
}

void console_write_at(uint16_t row, uint16_t col, const char* msg)
{
    uint16_t x = col;
    for (size_t i = 0; msg[i] != '\0' && x < VGA_WIDTH; ++i, ++x)
    {
        console_putc_at(row, x, msg[i]);
    }
}

void console_clear_line(uint16_t row)
{
    if (row >= VGA_HEIGHT)
    {
        return;
    }
    for (uint16_t x = 0; x < VGA_WIDTH; ++x)
    {
        VGA[row * VGA_WIDTH + x] = (uint16_t)vga_color << 8 | ' ';
    }
}

void console_get_cursor(uint16_t* row, uint16_t* col)
{
    if (row)
    {
        *row = vga_row;
    }
    if (col)
    {
        *col = vga_col;
    }
}

void console_set_cursor(uint16_t row, uint16_t col)
{
    if (row < VGA_HEIGHT)
    {
        vga_row = row;
    }
    if (col < VGA_WIDTH)
    {
        vga_col = col;
    }
}

void console_get_dimensions(uint16_t* width, uint16_t* height)
{
    if (width)
    {
        *width = VGA_WIDTH;
    }
    if (height)
    {
        *height = VGA_HEIGHT;
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
