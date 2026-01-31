#include "clipboard.h"

static char g_clipboard[CLIPBOARD_SIZE];
static size_t g_clipboard_len = 0;

void clipboard_init(void)
{
    g_clipboard_len = 0;
    for (int i = 0; i < CLIPBOARD_SIZE; i++)
    {
        g_clipboard[i] = 0;
    }
}

void clipboard_copy(const char* text)
{
    if (text == 0)
    {
        g_clipboard_len = 0;
        return;
    }

    size_t len = 0;
    while (text[len] != '\0' && len + 1 < CLIPBOARD_SIZE)
    {
        g_clipboard[len] = text[len];
        len++;
    }
    g_clipboard[len] = '\0';
    g_clipboard_len = len;
}

const char* clipboard_paste(void)
{
    return g_clipboard;
}

void clipboard_clear(void)
{
    g_clipboard_len = 0;
    for (int i = 0; i < CLIPBOARD_SIZE; i++)
    {
        g_clipboard[i] = 0;
    }
}
