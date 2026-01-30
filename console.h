#pragma once

#include <stddef.h>
#include <stdint.h>

void console_clear(void);
void console_putc(char c);
void console_write(const char* msg);
void console_backspace(void);
