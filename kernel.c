#include <stdint.h>
#include <stddef.h>

#if defined(__GNUC__)
#define ASM_VOLATILE(...) __asm__ __volatile__(__VA_ARGS__)
#else
#define ASM_VOLATILE(...)
#endif

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static const uint16_t VGA_WIDTH = 80;
static const uint16_t VGA_HEIGHT = 25;
static uint16_t vga_row = 0;
static uint16_t vga_col = 0;
static uint8_t vga_color = 0x0F;

static inline void outb(uint16_t port, uint8_t value)
{
    ASM_VOLATILE("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    ASM_VOLATILE("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void console_clear(void)
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

static void console_putc(char c)
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

static void console_write(const char* msg)
{
    for (size_t i = 0; msg[i] != '\0'; ++i)
    {
        console_putc(msg[i]);
    }
}

static void console_backspace(void)
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

static int str_equals(const char* a, const char* b)
{
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0')
    {
        if (a[i] != b[i])
        {
            return 0;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static const char* skip_spaces(const char* s)
{
    while (*s == ' ')
    {
        s++;
    }
    return s;
}

static const char scancode_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'', '`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static int keyboard_has_data(void)
{
    return (inb(0x64) & 0x01) != 0;
}

static char keyboard_read_char(void)
{
    uint8_t scancode = inb(0x60);
    if (scancode & 0x80)
    {
        return 0;
    }
    if (scancode < sizeof(scancode_map))
    {
        return scancode_map[scancode];
    }
    return 0;
}

static void print_prompt(void)
{
    console_write("> ");
}

static void execute_command(const char* line)
{
    const char* cmd = skip_spaces(line);
    if (*cmd == '\0')
    {
        return;
    }

    if (str_equals(cmd, "help"))
    {
        console_write("Commands: help, echo, clear, info\n");
        return;
    }

    if (str_equals(cmd, "clear"))
    {
        console_clear();
        return;
    }

    if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && (cmd[4] == ' ' || cmd[4] == '\0'))
    {
        const char* text = skip_spaces(cmd + 4);
        if (*text != '\0')
        {
            console_write(text);
        }
        console_putc('\n');
        return;
    }

    if (str_equals(cmd, "info"))
    {
        console_write("x86 kernel (32-bit, C, VGA)\n");
        return;
    }

    console_write("Unknown command. Type 'help'.\n");
}

void kernel_main(void)
{
    console_clear();
    console_write("Kernel C loaded.\n");
    print_prompt();

    char line[128];
    size_t len = 0;

    for (;;)
    {
        if (!keyboard_has_data())
        {
            continue;
        }

        char c = keyboard_read_char();
        if (c == 0)
        {
            continue;
        }

        if (c == '\b')
        {
            if (len > 0)
            {
                len--;
                line[len] = '\0';
                console_backspace();
            }
            continue;
        }

        if (c == '\n')
        {
            console_putc('\n');
            line[len] = '\0';
            execute_command(line);
            len = 0;
            line[0] = '\0';
            print_prompt();
            continue;
        }

        if (len + 1 < sizeof(line))
        {
            line[len++] = c;
            line[len] = '\0';
            console_putc(c);
        }
    }
}
