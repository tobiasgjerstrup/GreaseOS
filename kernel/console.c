#include "console.h"
#include "framebuffer.h"
#include "font8x16.h"

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static const uint16_t VGA_WIDTH = 80;
static const uint16_t VGA_HEIGHT = 25;
static uint16_t vga_row = 0;
static uint16_t vga_col = 0;
static uint8_t vga_color = 0x0F;

static int use_fb = 0;
static uint16_t fb_row = 0;
static uint16_t fb_col = 0;
static uint16_t fb_width = 0;
static uint16_t fb_height = 0;
static uint32_t fb_fg_color = 0xAAAAAA;
static uint32_t fb_bg_color = 0x0D0D12;

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 16

static void fb_draw_glyph(uint16_t row, uint16_t col, char c, uint32_t fg, uint32_t bg)
{
    if (!fb_is_available())
    {
        return;
    }

    uint8_t ch = (uint8_t)c;
    if (ch < 32 || ch > 126)
    {
        ch = 32;
    }
    const uint8_t *glyph = font8x16[ch - 32];

    uint32_t pixel_x = col * GLYPH_WIDTH;
    uint32_t pixel_y = row * GLYPH_HEIGHT;

    for (uint32_t y = 0; y < GLYPH_HEIGHT; y++)
    {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < GLYPH_WIDTH; x++)
        {
            uint32_t color = (line & (0x80 >> x)) ? fg : bg;
            fb_put_pixel(pixel_x + x, pixel_y + y, color);
        }
    }
}

static void fb_scroll_console(void)
{
    if (!fb_is_available())
    {
        return;
    }

    const struct fb_info *info = fb_get_info();
    if (!info)
    {
        return;
    }

    uint8_t *base = (uint8_t *)(uintptr_t)info->address;
    uint32_t bytes_per_pixel = info->bpp / 8;

    for (uint32_t y = 0; y < (uint32_t)((fb_height - 1) * GLYPH_HEIGHT); y++)
    {
        uint8_t *dest = base + y * info->pitch;
        uint8_t *src = base + (y + GLYPH_HEIGHT) * info->pitch;
        for (uint32_t x = 0; x < info->width * bytes_per_pixel; x++)
        {
            dest[x] = src[x];
        }
    }

    for (uint32_t y = (fb_height - 1) * GLYPH_HEIGHT; y < info->height; y++)
    {
        for (uint32_t x = 0; x < info->width; x++)
        {
            fb_put_pixel(x, y, fb_bg_color);
        }
    }
}

void console_use_framebuffer(void)
{
    if (!fb_is_available())
    {
        return;
    }

    const struct fb_info *info = fb_get_info();
    if (!info)
    {
        return;
    }

    use_fb = 1;
    fb_width = info->width / GLYPH_WIDTH;
    fb_height = info->height / GLYPH_HEIGHT;
    fb_row = 0;
    fb_col = 0;
}

void console_set_colors(uint32_t fg, uint32_t bg)
{
    fb_fg_color = fg;
    fb_bg_color = bg;
}

static void console_scroll(void)
{
    if (use_fb)
    {
        fb_scroll_console();
        if (fb_row > 0)
        {
            fb_row--;
        }
        return;
    }

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
    if (use_fb)
    {
        fb_clear(fb_bg_color);
        fb_row = 0;
        fb_col = 0;
        return;
    }

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
    if (use_fb)
    {
        if (c == '\n')
        {
            fb_col = 0;
            fb_row++;
            if (fb_row >= fb_height)
            {
                console_scroll();
            }
            return;
        }

        fb_draw_glyph(fb_row, fb_col, c, fb_fg_color, fb_bg_color);
        fb_col++;
        if (fb_col >= fb_width)
        {
            fb_col = 0;
            fb_row++;
            if (fb_row >= fb_height)
            {
                console_scroll();
            }
        }
        return;
    }

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
    if (use_fb)
    {
        if (row >= fb_height || col >= fb_width)
        {
            return;
        }
        fb_draw_glyph(row, col, c, fb_fg_color, fb_bg_color);
        return;
    }

    if (row >= VGA_HEIGHT || col >= VGA_WIDTH)
    {
        return;
    }
    VGA[row * VGA_WIDTH + col] = (uint16_t)vga_color << 8 | (uint8_t)c;
}

void console_write_at(uint16_t row, uint16_t col, const char* msg)
{
    if (use_fb)
    {
        uint16_t x = col;
        for (size_t i = 0; msg[i] != '\0' && x < fb_width; ++i, ++x)
        {
            console_putc_at(row, x, msg[i]);
        }
        return;
    }

    uint16_t x = col;
    for (size_t i = 0; msg[i] != '\0' && x < VGA_WIDTH; ++i, ++x)
    {
        console_putc_at(row, x, msg[i]);
    }
}

void console_clear_line(uint16_t row)
{
    if (use_fb)
    {
        if (row >= fb_height)
        {
            return;
        }
        for (uint16_t x = 0; x < fb_width; ++x)
        {
            fb_draw_glyph(row, x, ' ', fb_fg_color, fb_bg_color);
        }
        return;
    }

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
    if (use_fb)
    {
        if (row)
        {
            *row = fb_row;
        }
        if (col)
        {
            *col = fb_col;
        }
        return;
    }

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
    if (use_fb)
    {
        if (row < fb_height)
        {
            fb_row = row;
        }
        if (col < fb_width)
        {
            fb_col = col;
        }
        return;
    }

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
    if (use_fb)
    {
        if (width)
        {
            *width = fb_width;
        }
        if (height)
        {
            *height = fb_height;
        }
        return;
    }

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
    if (use_fb)
    {
        if (fb_col == 0 && fb_row == 0)
        {
            return;
        }

        if (fb_col == 0)
        {
            fb_row--;
            fb_col = fb_width - 1;
        }
        else
        {
            fb_col--;
        }

        fb_draw_glyph(fb_row, fb_col, ' ', fb_fg_color, fb_bg_color);
        return;
    }

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

char console_get_char_at(uint16_t row, uint16_t col)
{
    if (use_fb)
    {
        return ' ';
    }

    if (row >= VGA_HEIGHT || col >= VGA_WIDTH)
    {
        return ' ';
    }
    uint16_t data = VGA[row * VGA_WIDTH + col];
    return (char)(data & 0xFF);
}

char* console_get_line(uint16_t row, char* buf, size_t max_len)
{
    if (use_fb)
    {
        if (buf && max_len > 0)
        {
            buf[0] = '\0';
        }
        return buf;
    }

    if (row >= VGA_HEIGHT || buf == 0 || max_len == 0)
    {
        if (buf && max_len > 0)
        {
            buf[0] = '\0';
        }
        return buf;
    }

    size_t i = 0;
    for (uint16_t col = 0; col < VGA_WIDTH && i + 1 < max_len; col++)
    {
        char ch = console_get_char_at(row, col);
        if (ch == ' ')
        {
            break;
        }
        buf[i++] = ch;
    }
    buf[i] = '\0';
    return buf;
}
