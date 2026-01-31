#include "mouse.h"
#include "io.h"

#define MOUSE_ACK 0xFA
#define MOUSE_DATA_PORT 0x60
#define MOUSE_CMD_PORT 0x64
#define MOUSE_STATUS_PORT 0x64

static int16_t g_mouse_x = 0;
static int16_t g_mouse_y = 0;
static uint8_t g_mouse_buttons = 0;
static uint8_t g_packet_index = 0;
static uint8_t g_packet[3];

static void mouse_write_cmd(uint8_t cmd)
{
    while (inb(MOUSE_STATUS_PORT) & 0x02)
    {
    }
    outb(MOUSE_CMD_PORT, 0xD4);

    while (inb(MOUSE_STATUS_PORT) & 0x02)
    {
    }
    outb(MOUSE_DATA_PORT, cmd);
}

static uint8_t mouse_read_response(void)
{
    while ((inb(MOUSE_STATUS_PORT) & 0x01) == 0)
    {
    }
    return inb(MOUSE_DATA_PORT);
}

void mouse_init(void)
{
    g_mouse_x = 0;
    g_mouse_y = 0;
    g_mouse_buttons = 0;
    g_packet_index = 0;

    mouse_write_cmd(0xE8);
    mouse_read_response();

    mouse_write_cmd(0x03);
    mouse_read_response();

    mouse_write_cmd(0xE7);
    mouse_read_response();

    mouse_write_cmd(0xE6);
    mouse_read_response();

    mouse_write_cmd(0xF4);
    mouse_read_response();
}

int mouse_has_data(void)
{
    return (inb(MOUSE_STATUS_PORT) & 0x01) != 0;
}

mouse_state_t mouse_read(void)
{
    mouse_state_t state;
    state.x = g_mouse_x;
    state.y = g_mouse_y;
    state.buttons = g_mouse_buttons;

    if (mouse_has_data())
    {
        uint8_t data = inb(MOUSE_DATA_PORT);

        if (g_packet_index == 0 && (data & 0xC0) != 0)
        {
            return state;
        }

        g_packet[g_packet_index++] = data;

        if (g_packet_index >= 3)
        {
            g_packet_index = 0;

            g_mouse_buttons = g_packet[0] & 0x07;

            int8_t dx = (int8_t)g_packet[1];
            int8_t dy = (int8_t)g_packet[2];

            if (g_packet[0] & 0x10)
            {
                dx |= 0xFF00;
            }
            if (g_packet[0] & 0x20)
            {
                dy |= 0xFF00;
            }

            g_mouse_x += dx;
            g_mouse_y -= dy;

            if (g_mouse_x < 0)
                g_mouse_x = 0;
            if (g_mouse_y < 0)
                g_mouse_y = 0;
            if (g_mouse_x > 640)
                g_mouse_x = 640;
            if (g_mouse_y > 200)
                g_mouse_y = 200;

            state.x = g_mouse_x;
            state.y = g_mouse_y;
            state.buttons = g_mouse_buttons;
        }
    }

    return state;
}
