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

int keyboard_has_data(void)
{
    return (inb(0x64) & 0x01) != 0;
}

int keyboard_read_key(void)
{
    uint8_t scancode = inb(0x60);

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
