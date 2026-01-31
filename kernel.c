#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "fs/fat.h"
#include "io.h"
#include "keyboard.h"
#include "editor.h"
#include "hwinfo.h"
#include "exec.h"
#include "snake.h"

static const char *skip_spaces(const char *s)
{
    while (*s == ' ')
    {
        s++;
    }
    return s;
}

#define HISTORY_MAX 20
#define HISTORY_SIZE 128
static char g_history[HISTORY_MAX][HISTORY_SIZE];
static int g_history_count = 0;
static int g_history_head = 0;
static int g_history_index = -1;

static void history_add(const char *cmd)
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

static const char *history_up(void)
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

static const char *history_down(void)
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

static void shutdown(void)
{
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
}

static void restart(void)
{
    while (inb(0x64) & 0x02)
    {
    }
    outb(0x64, 0xFE);
}

static int cmd_is(const char *cmd, size_t len, const char *word)
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

static void execute_command(const char *line)
{
    const char *cmd = skip_spaces(line);
    if (*cmd == '\0')
    {
        return;
    }

    size_t cmd_len = 0;
    while (cmd[cmd_len] != '\0' && cmd[cmd_len] != ' ')
    {
        cmd_len++;
    }
    const char *arg = skip_spaces(cmd + cmd_len);

    if (cmd_is(cmd, cmd_len, "help"))
    {
        console_write("Commands: help, echo, clear, info, hw, ls, cd, pwd, mkdir, rmdir, touch, cat, write, rm, v, exec, ss, df, snake, shutdown, restart\n");
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
#if defined(__x86_64__) || defined(__amd64__)
        console_write("x86 kernel (64-bit, C, VGA)\n");
#else
        console_write("x86 kernel (32-bit, C, VGA)\n");
#endif
        return;
    }

    if (cmd_is(cmd, cmd_len, "hw"))
    {
        hwinfo_display();
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

    if (cmd_is(cmd, cmd_len, "rmdir"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: rmdir <name>\n");
            return;
        }
        if (fat_rmdir(arg) != 0)
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

    if (cmd_is(cmd, cmd_len, "rm"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: rm <name>\n");
            return;
        }
        if (fat_rm(arg) != 0)
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
        const char *text = skip_spaces(arg + i);
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

    if (cmd_is(cmd, cmd_len, "v"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: v <name>\n");
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
        if (name[0] == '\0')
        {
            console_write("Usage: v <name>\n");
            return;
        }
        if (editor_run(name) != 0)
        {
            console_write("Editor error\n");
        }
        return;
    }

    if (cmd_is(cmd, cmd_len, "exec"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: exec <file>\n");
            return;
        }
        exec_run(arg);
        return;
    }

    if (cmd_is(cmd, cmd_len, "ss"))
    {
        if (*arg == '\0')
        {
            console_write("Usage: ss <file>\n");
            return;
        }
        char filename[13];
        size_t i = 0;
        while (arg[i] != '\0' && arg[i] != ' ' && i + 1 < sizeof(filename))
        {
            filename[i] = arg[i];
            i++;
        }
        filename[i] = '\0';
        
        char script_buffer[4096];
        size_t script_size = 0;
        
        if (fat_read(filename, script_buffer, sizeof(script_buffer) - 1, &script_size) != 0)
        {
            console_write("ss: ");
            console_write(fat_last_error());
            console_putc('\n');
            return;
        }
        
        script_buffer[script_size] = '\0';
        
        size_t line_start = 0;
        size_t pos = 0;
        
        while (pos <= script_size)
        {
            if (pos == script_size || script_buffer[pos] == '\n')
            {
                char line_buffer[128];
                size_t line_len = pos - line_start;
                
                if (line_len > 0 && script_buffer[line_start + line_len - 1] == '\r')
                {
                    line_len--;
                }
                
                if (line_len >= sizeof(line_buffer))
                {
                    line_len = sizeof(line_buffer) - 1;
                }
                
                for (size_t j = 0; j < line_len; j++)
                {
                    line_buffer[j] = script_buffer[line_start + j];
                }
                line_buffer[line_len] = '\0';
                
                const char *trimmed = skip_spaces(line_buffer);
                if (trimmed[0] != '\0' && trimmed[0] != '#')
                {
                    console_write(">> ");
                    console_write(trimmed);
                    console_putc('\n');
                    execute_command(trimmed);
                }
                
                line_start = pos + 1;
            }
            pos++;
        }
        
        console_write("ss: script finished\n");
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

    if (cmd_is(cmd, cmd_len, "snake"))
    {
        snake_game_run();
        console_clear();
        console_write("Kernel C loaded.\n");
#if defined(__x86_64__) || defined(__amd64__)
        console_write("x86 kernel (64-bit, C, VGA)\n");
#else
        console_write("x86 kernel (32-bit, C, VGA)\n");
#endif
        print_prompt();
        return;
    }

    if (cmd_is(cmd, cmd_len, "shutdown"))
    {
        console_write("Shutting down...\n");
        shutdown();
        return;
    }

    if (cmd_is(cmd, cmd_len, "restart"))
    {
        console_write("Restarting...\n");
        restart();
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
#if defined(__x86_64__) || defined(__amd64__)
    console_write("x86 kernel (64-bit, C, VGA)\n");
#else
    console_write("x86 kernel (32-bit, C, VGA)\n");
#endif
    print_prompt();

    char line[128];
    size_t len = 0;

    for (;;)
    {
        if (!keyboard_has_data())
        {
            continue;
        }

        int c = keyboard_read_key();
        if (c == 0)
        {
            continue;
        }

        if (c == KEY_UP)
        {
            const char *hist = history_up();
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

        if (c == KEY_DOWN)
        {
            const char *hist = history_down();
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

        if (c >= KEY_UP)
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
            line[len++] = (char)c;
            line[len] = '\0';
            console_putc((char)c);
        }
    }
}
