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


#define HISTORY_MAX 20
#define HISTORY_SIZE 128
static char g_history[HISTORY_MAX][HISTORY_SIZE];
static int g_history_count = 0;
static int g_history_head = 0;
static int g_history_index = -1;
static uint8_t g_extended_scancode = 0;

static int keyboard_has_data(void)
{
    return (inb(0x64) & 0x01) != 0;
}

static char keyboard_read_char(void)
{
    uint8_t scancode = inb(0x60);
    
    if (scancode == 0xE0)
    {
        g_extended_scancode = 1;
        return 0;
    }
    
    if (g_extended_scancode)
    {
        g_extended_scancode = 0;
        if (scancode == 0x48)
        {
            return 1;
        }
        if (scancode == 0x50)
        {
            return 2;
        }
        return 0;
    }
    
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

static void history_add(const char* cmd)
{
    size_t i = 0;
    while (cmd[i] != '\0' && i + 1 < HISTORY_SIZE)
    {
        g_history[g_history_head][i] = cmd[i];
        i++;
    }
    g_history[g_history_head][i] = '\0';

    g_history_head = (g_history_head + 1) % HISTORY_MAX;
    if (g_history_count < HISTORY_MAX)
    {
        g_history_count++;
    }
    g_history_index = -1;
}

static const char* history_up(void)
{
    if (g_history_count == 0)
    {
        return "";
    }
    if (g_history_index == -1)
    {
        g_history_index = (g_history_head - 1 + HISTORY_MAX) % HISTORY_MAX;
    }
    else
    {
        int next = (g_history_index - 1 + HISTORY_MAX) % HISTORY_MAX;
        int oldest = (g_history_head - g_history_count + HISTORY_MAX) % HISTORY_MAX;
        if (g_history_index != oldest)
        {
            g_history_index = next;
        }
    }
    return g_history[g_history_index];
}

static const char* history_down(void)
{
    if (g_history_count == 0 || g_history_index == -1)
    {
        return "";
    }
    int newest = (g_history_head - 1 + HISTORY_MAX) % HISTORY_MAX;
    if (g_history_index == newest)
    {
        g_history_index = -1;
        return "";
    }
    g_history_index = (g_history_index + 1) % HISTORY_MAX;
    return g_history[g_history_index];
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
        console_write("Commands: help, echo, clear, info, ls, cd, pwd, mkdir, touch, cat, write, df\n");
        console_write("Use UP/DOWN arrow keys to navigate command history.\n");
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

    if (cmd_is(cmd, cmd_len, "df"))
    {
        if (fat_df() != 0)
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

        if (c == 1)  // up arrow
        {
            const char* hist = history_up();
            for (size_t i = 0; i < len; i++)
            {
                console_backspace();
            }
            len = 0;
            while (hist[len] != '\0' && len + 1 < sizeof(line))
            {
                line[len] = hist[len];
                console_putc(hist[len]);
                len++;
            }
            line[len] = '\0';
            continue;
        }

        if (c == 2)  // down arrow
        {
            const char* hist = history_down();
            for (size_t i = 0; i < len; i++)
            {
                console_backspace();
            }
            len = 0;
            while (hist[len] != '\0' && len + 1 < sizeof(line))
            {
                line[len] = hist[len];
                console_putc(hist[len]);
                len++;
            }
            line[len] = '\0';
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
            if (line[0] != '\0')
            {
                history_add(line);
            }
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
