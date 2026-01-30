#include <stdint.h>

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;

static void vga_write(const char* msg, uint8_t color)
{
    for (uint16_t i = 0; msg[i] != '\0'; ++i)
    {
        VGA[i] = (uint16_t)color << 8 | msg[i];
    }
}

void kernel_main(void)
{
    vga_write("Kernel C loaded.", 0x0F);

    for (;;)
    {
        __asm__ __volatile__("hlt");
    }
}
