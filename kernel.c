#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "fs/fat.h"
#include "io.h"

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

static int cmd_is(const char* cmd, size_t len, const char* word)
{
    size_t i = 0;
    while (word[i] != '\0')
    {
        if (i >= len || cmd[i] != word[i])
        {
            return 0;
        }
        i++;
    }
    return i == len;
}

static void execute_command(const char* line)
{
    const char* cmd = skip_spaces(line);
    if (*cmd == '\0')
    {
        return;
    }

    size_t cmd_len = 0;
    while (cmd[cmd_len] != '\0' && cmd[cmd_len] != ' ')
    {
        cmd_len++;
    }
    const char* arg = skip_spaces(cmd + cmd_len);

    if (cmd_is(cmd, cmd_len, "help"))
    {
        console_write("Commands: help, echo, clear, info, ls, cd, pwd, mkdir, touch, cat, write\n");
        return;
    }

    if (cmd_is(cmd, cmd_len, "clear"))
    {
        console_clear();
        return;
    }

    if (cmd_is(cmd, cmd_len, "echo"))
    {
        console_write(arg);
        console_putc('\n');
        return;
    }

    if (cmd_is(cmd, cmd_len, "info"))
    {
        console_write("x86 kernel (32-bit, C, VGA)\n");
        return;
    }

    if (cmd_is(cmd, cmd_len, "ls"))
    {
        if (fat_ls() != 0)
        {
            console_write(fat_last_error());
            console_putc('\n');
        }
        return;
    }

    if (cmd_is(cmd, cmd_len, "pwd"))
    {
        console_write(fat_pwd());
        console_putc('\n');
        return;
    }

    if (cmd_is(cmd, cmd_len, "cd"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: cd <dir>\n");
            return;
        }
        if (fat_cd(arg) != 0)
        {
            console_write(fat_last_error());
            console_putc('\n');
        }
        return;
    }

    if (cmd_is(cmd, cmd_len, "mkdir"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: mkdir <name>\n");
            return;
        }
        if (fat_mkdir(arg) != 0)
        {
            console_write(fat_last_error());
            console_putc('\n');
        }
        return;
    }

    if (cmd_is(cmd, cmd_len, "touch"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: touch <name>\n");
            return;
        }
        if (fat_touch(arg) != 0)
        {
            console_write(fat_last_error());
            console_putc('\n');
        }
        return;
    }

    if (cmd_is(cmd, cmd_len, "cat"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: cat <name>\n");
            return;
        }
        if (fat_cat(arg) != 0)
        {
            console_write(fat_last_error());
            console_putc('\n');
        }
        return;
    }

    if (cmd_is(cmd, cmd_len, "write"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: write <name> <text>\n");
            return;
        }
        char name[13];
        size_t i = 0;
        while (arg[i] != '\0' && arg[i] != ' ' && i + 1 < sizeof(name))
        {
            name[i] = arg[i];
            i++;
        }
        name[i] = '\0';
        const char* text = skip_spaces(arg + i);
        if (name[0] == '\0')
        {
            console_write("Usage: write <name> <text>\n");
            return;
        }
        if (fat_write(name, text) != 0)
        {
            console_write(fat_last_error());
            console_putc('\n');
        }
        return;
    }

    console_write("Unknown command. Type 'help'.\n");
}

void kernel_main(void)
{
    console_clear();
    console_write("Kernel C loaded.\n");
    if (fat_init() != 0)
    {
        console_write("FAT init failed: ");
        console_write(fat_last_error());
        console_putc('\n');
    }
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
