#pragma once

#include <stddef.h>

#define CLIPBOARD_SIZE 4096

void clipboard_init(void);
void clipboard_copy(const char* text);
const char* clipboard_paste(void);
void clipboard_clear(void);
