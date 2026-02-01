#include <stdint.h>

#include "io.h"
#include "keyboard.h"

static const char scancode_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'', '`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static uint8_t g_extended_scancode = 0;
static uint8_t g_ctrl_down = 0;
static uint8_t g_last_scancode = 0;
static uint8_t g_last_status = 0;

static void keyboard_wait_write(void)
{
    while (inb(0x64) & 0x02)
    {
    }
}

static void keyboard_wait_read(void)
{
    while ((inb(0x64) & 0x01) == 0)
    {
    }
}

void keyboard_init(void)
{
    while (inb(0x64) & 0x01)
    {
        (void)inb(0x60);
    }

    keyboard_wait_write();
    outb(0x64, 0xAD);
    keyboard_wait_write();
    outb(0x64, 0xA7);

    keyboard_wait_write();
    outb(0x64, 0x20);
    keyboard_wait_read();
    uint8_t config = inb(0x60);
    config |= 0x01;
    config &= ~(1 << 4);
    config &= ~(1 << 5);
    config |= (1 << 6);

    keyboard_wait_write();
    outb(0x64, 0x60);
    keyboard_wait_write();
    outb(0x60, config);

    keyboard_wait_write();
    outb(0x64, 0xAE);

    keyboard_wait_write();
    outb(0x60, 0xF4);

    keyboard_wait_read();
    (void)inb(0x60);
}

int keyboard_has_data(void)
{
    g_last_status = inb(0x64);
    return (g_last_status & 0x01) != 0;
}

int keyboard_read_key(void)
{
    uint8_t scancode = inb(0x60);
    g_last_scancode = scancode;

    if (scancode == 0xE0)
    {
        g_extended_scancode = 1;
        return 0;
    }

    if (scancode == 0x1D)
    {
        g_ctrl_down = 1;
        return 0;
    }
    if (scancode == 0x9D)
    {
        g_ctrl_down = 0;
        return 0;
    }

    if (g_extended_scancode)
    {
        g_extended_scancode = 0;
        if (scancode == 0x48)
        {
            return KEY_UP;
        }
        if (scancode == 0x50)
        {
            return KEY_DOWN;
        }
        if (scancode == 0x4B)
        {
            return KEY_LEFT;
        }
        if (scancode == 0x4D)
        {
            return KEY_RIGHT;
        }
        return 0;
    }

    if (scancode & 0x80)
    {
        return 0;
    }

    if (scancode < sizeof(scancode_map))
    {
        char ch = scancode_map[scancode];
        if (ch == 0)
        {
            return 0;
        }
        if (g_ctrl_down && ch >= 'a' && ch <= 'z')
        {
            return (ch - 'a') + 1;
        }
        return (int)ch;
    }

    return 0;
}

uint8_t keyboard_last_scancode(void)
{
    return g_last_scancode;
}

uint8_t keyboard_last_status(void)
{
    return g_last_status;
}
