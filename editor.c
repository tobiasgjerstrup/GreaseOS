#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "editor.h"
#include "fs/fat.h"
#include "keyboard.h"

#define EDITOR_MAX_SIZE 16384
#define STATUS_MSG_MAX 64
#define FILENAME_MAX 32

static char g_buffer[EDITOR_MAX_SIZE];
static size_t g_len = 0;
static size_t g_cursor = 0;
static uint32_t g_scroll_row = 0;
static uint8_t g_dirty = 0;
static uint8_t g_quit_confirm = 0;
static char g_status[STATUS_MSG_MAX];
static char g_filename[FILENAME_MAX];

static size_t str_len(const char* s)
{
    size_t i = 0;
    while (s[i] != '\0')
    {
        i++;
    }
    return i;
}

static int str_eq(const char* a, const char* b)
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
    return a[i] == b[i];
}

static void str_copy(char* dst, size_t dst_len, const char* src)
{
    size_t i = 0;
    if (dst_len == 0)
    {
        return;
    }
    while (src[i] != '\0' && i + 1 < dst_len)
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void mem_move(char* dst, const char* src, size_t len)
{
    if (dst == src || len == 0)
    {
        return;
    }
    if (dst < src)
    {
        for (size_t i = 0; i < len; ++i)
        {
            dst[i] = src[i];
        }
    }
    else
    {
        for (size_t i = len; i > 0; --i)
        {
            dst[i - 1] = src[i - 1];
        }
    }
}

static void u32_to_str(uint32_t value, char* out, size_t out_len)
{
    if (out_len == 0)
    {
        return;
    }

    char temp[16];
    size_t idx = 0;
    if (value == 0)
    {
        temp[idx++] = '0';
    }
    else
    {
        while (value > 0 && idx < sizeof(temp))
        {
            temp[idx++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    size_t out_idx = 0;
    while (idx > 0 && out_idx + 1 < out_len)
    {
        out[out_idx++] = temp[--idx];
    }
    out[out_idx] = '\0';
}

static void editor_set_status(const char* msg)
{
    str_copy(g_status, sizeof(g_status), msg);
}

static void editor_get_visual_pos(size_t index, uint16_t width, uint32_t* out_row, uint16_t* out_col)
{
    uint32_t row = 0;
    uint16_t col = 0;
    for (size_t i = 0; i < index && i < g_len; ++i)
    {
        char c = g_buffer[i];
        if (c == '\n')
        {
            row++;
            col = 0;
        }
        else
        {
            col++;
            if (col >= width)
            {
                row++;
                col = 0;
            }
        }
    }
    *out_row = row;
    *out_col = col;
}

static size_t editor_index_for_row(uint32_t target_row, uint16_t width)
{
    uint32_t row = 0;
    uint16_t col = 0;
    for (size_t i = 0; i < g_len; ++i)
    {
        if (row == target_row && col == 0)
        {
            return i;
        }
        char c = g_buffer[i];
        if (c == '\n')
        {
            row++;
            col = 0;
            continue;
        }
        col++;
        if (col >= width)
        {
            row++;
            col = 0;
        }
    }
    if (row == target_row)
    {
        return g_len;
    }
    return g_len;
}

static size_t editor_index_for_visual(uint32_t target_row, uint16_t target_col, uint16_t width)
{
    uint32_t row = 0;
    uint16_t col = 0;
    for (size_t i = 0; i < g_len; ++i)
    {
        if (row == target_row && col == target_col)
        {
            return i;
        }
        char c = g_buffer[i];
        if (c == '\n')
        {
            if (row == target_row)
            {
                return i;
            }
            row++;
            col = 0;
            continue;
        }
        col++;
        if (col >= width)
        {
            row++;
            col = 0;
            if (row > target_row)
            {
                return i + 1;
            }
        }
    }
    if (row == target_row)
    {
        return g_len;
    }
    return g_len;
}

static void editor_scroll_to_cursor(uint16_t width, uint16_t height)
{
    uint32_t row = 0;
    uint16_t col = 0;
    editor_get_visual_pos(g_cursor, width, &row, &col);
    uint16_t text_height = (uint16_t)(height - 1);

    if (row < g_scroll_row)
    {
        g_scroll_row = row;
    }
    else if (row >= g_scroll_row + text_height)
    {
        g_scroll_row = row - text_height + 1;
    }
}

static void editor_render(void)
{
    uint16_t width = 0;
    uint16_t height = 0;
    console_get_dimensions(&width, &height);

    editor_scroll_to_cursor(width, height);

    for (uint16_t r = 0; r < height; ++r)
    {
        console_clear_line(r);
    }

    uint16_t text_height = (uint16_t)(height - 1);
    size_t idx = editor_index_for_row(g_scroll_row, width);
    uint16_t row = 0;
    uint16_t col = 0;

    while (idx < g_len && row < text_height)
    {
        char c = g_buffer[idx++];
        if (c == '\n')
        {
            row++;
            col = 0;
            continue;
        }
        console_putc_at(row, col, c);
        col++;
        if (col >= width)
        {
            row++;
            col = 0;
        }
    }

    uint32_t cur_row = 0;
    uint16_t cur_col = 0;
    editor_get_visual_pos(g_cursor, width, &cur_row, &cur_col);
    if (cur_row >= g_scroll_row && cur_row < g_scroll_row + text_height)
    {
        uint16_t screen_row = (uint16_t)(cur_row - g_scroll_row);
        console_putc_at(screen_row, cur_col, '_');
    }

    char status_line[128];
    char num_buf[16];
    str_copy(status_line, sizeof(status_line), "v ");
    str_copy(status_line + str_len(status_line), sizeof(status_line) - str_len(status_line), g_filename);
    str_copy(status_line + str_len(status_line), sizeof(status_line) - str_len(status_line), "  ");

    u32_to_str(cur_row + 1, num_buf, sizeof(num_buf));
    str_copy(status_line + str_len(status_line), sizeof(status_line) - str_len(status_line), "Ln ");
    str_copy(status_line + str_len(status_line), sizeof(status_line) - str_len(status_line), num_buf);
    str_copy(status_line + str_len(status_line), sizeof(status_line) - str_len(status_line), " Col ");
    u32_to_str((uint32_t)cur_col + 1, num_buf, sizeof(num_buf));
    str_copy(status_line + str_len(status_line), sizeof(status_line) - str_len(status_line), num_buf);

    if (g_dirty)
    {
        str_copy(status_line + str_len(status_line), sizeof(status_line) - str_len(status_line), "  *");
    }

    console_clear_line((uint16_t)(height - 1));
    console_write_at((uint16_t)(height - 1), 0, status_line);

    if (g_status[0] != '\0')
    {
        console_write_at((uint16_t)(height - 1), (uint16_t)(width > 30 ? width - 30 : 0), g_status);
    }
}

static void editor_insert_char(char c)
{
    if (g_len + 1 >= EDITOR_MAX_SIZE)
    {
        editor_set_status("Buffer full");
        return;
    }

    mem_move(&g_buffer[g_cursor + 1], &g_buffer[g_cursor], g_len - g_cursor);
    g_buffer[g_cursor] = c;
    g_cursor++;
    g_len++;
    g_buffer[g_len] = '\0';
    g_dirty = 1;
    g_quit_confirm = 0;
}

static void editor_backspace(void)
{
    if (g_cursor == 0)
    {
        return;
    }

    mem_move(&g_buffer[g_cursor - 1], &g_buffer[g_cursor], g_len - g_cursor);
    g_cursor--;
    g_len--;
    g_buffer[g_len] = '\0';
    g_dirty = 1;
    g_quit_confirm = 0;
}

static void editor_move_left(void)
{
    if (g_cursor > 0)
    {
        g_cursor--;
    }
}

static void editor_move_right(void)
{
    if (g_cursor < g_len)
    {
        g_cursor++;
    }
}

static void editor_move_up(void)
{
    uint16_t width = 0;
    console_get_dimensions(&width, 0);

    uint32_t row = 0;
    uint16_t col = 0;
    editor_get_visual_pos(g_cursor, width, &row, &col);
    if (row == 0)
    {
        return;
    }
    g_cursor = editor_index_for_visual(row - 1, col, width);
}

static void editor_move_down(void)
{
    uint16_t width = 0;
    console_get_dimensions(&width, 0);

    uint32_t row = 0;
    uint16_t col = 0;
    editor_get_visual_pos(g_cursor, width, &row, &col);
    g_cursor = editor_index_for_visual(row + 1, col, width);
}

static int editor_save(void)
{
    if (fat_write_data(g_filename, g_buffer, g_len) != 0)
    {
        editor_set_status(fat_last_error());
        return -1;
    }
    g_dirty = 0;
    editor_set_status("Saved");
    return 0;
}

static int editor_load(const char* filename)
{
    size_t out_size = 0;
    if (fat_read(filename, g_buffer, sizeof(g_buffer), &out_size) != 0)
    {
        if (str_eq(fat_last_error(), "Not found"))
        {
            g_len = 0;
            g_buffer[0] = '\0';
            g_cursor = 0;
            g_dirty = 0;
            editor_set_status("New file");
            return 0;
        }
        return -1;
    }

    g_len = out_size;
    g_buffer[g_len] = '\0';
    g_cursor = 0;
    g_dirty = 0;
    editor_set_status("");
    return 0;
}

int editor_run(const char* filename)
{
    str_copy(g_filename, sizeof(g_filename), filename);
    g_scroll_row = 0;
    g_status[0] = '\0';
    g_quit_confirm = 0;

    if (editor_load(g_filename) != 0)
    {
        console_write("Editor error: ");
        console_write(fat_last_error());
        console_putc('\n');
        return -1;
    }

    console_clear();
    editor_render();

    for (;;)
    {
        if (!keyboard_has_data())
        {
            continue;
        }

        int key = keyboard_read_key();
        if (key == 0)
        {
            continue;
        }

        if (key == KEY_UP)
        {
            editor_move_up();
        }
        else if (key == KEY_DOWN)
        {
            editor_move_down();
        }
        else if (key == KEY_LEFT)
        {
            editor_move_left();
        }
        else if (key == KEY_RIGHT)
        {
            editor_move_right();
        }
        else if (key == '\b')
        {
            editor_backspace();
        }
        else if (key == '\n')
        {
            editor_insert_char('\n');
        }
        else if (key == 0x13)
        {
            editor_save();
        }
        else if (key == 0x11)
        {
            if (g_dirty && !g_quit_confirm)
            {
                editor_set_status("Unsaved (Ctrl+Q again)");
                g_quit_confirm = 1;
            }
            else
            {
                break;
            }
        }
        else if (key >= 32 && key <= 126)
        {
            editor_insert_char((char)key);
        }

        editor_render();
    }

    console_clear();
    return 0;
}
