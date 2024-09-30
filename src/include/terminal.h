#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

void terminal_clear();
void print(const char *str);
void print_color(const char *str, uint8_t color);
void print_line(const char *str);

void sprint(const char *str, int max);
void sprint_line(const char *str, int max);

#endif