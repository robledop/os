#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <stdint.h>
#include <termcolors.h>
#include <printf.h>

#define VIDEO_MEMORY 0xB8000
#define DEFAULT_ATTRIBUTE 0x07 // Light grey on black background

void vga_buffer_init();
void print(const char str[static 1]);
void terminal_clear();
void terminal_putchar(char c, uint8_t attr, int x, int y);
// void printf(const char fmt[static 1], ...);
void update_cursor(int row, int col);
void putchar_(char c);
