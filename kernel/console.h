#pragma once

#include <stddef.h>
#include <stdint.h>

void console_clear(void);
void console_putc(char c);
void console_write(const char* msg);
void console_backspace(void);
void console_putc_at(uint16_t row, uint16_t col, char c);
void console_write_at(uint16_t row, uint16_t col, const char* msg);
void console_clear_line(uint16_t row);
void console_get_cursor(uint16_t* row, uint16_t* col);
void console_set_cursor(uint16_t row, uint16_t col);
void console_get_dimensions(uint16_t* width, uint16_t* height);
char console_get_char_at(uint16_t row, uint16_t col);
char* console_get_line(uint16_t row, char* buf, size_t max_len);

void console_use_framebuffer(void);
void console_set_colors(uint32_t fg, uint32_t bg);
