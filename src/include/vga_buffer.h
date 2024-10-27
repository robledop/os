#pragma once

#include "types.h"
#include "termcolors.h"


#define VIDEO_MEMORY 0xB8000
#define DEFAULT_ATTRIBUTE 0x07 // Light grey on black background

void vga_buffer_init();
void print(const char *str);
void terminal_clear();
// void terminal_write_char(char c, uint8_t fcolor, uint8_t bcolor);
void terminal_putchar(const char c, const uint8_t attr, const int x, const int y);
void kprintf(const char *fmt, ...);

void ksprintf(const char *str, int max);
void update_cursor(const int row, const int col);
