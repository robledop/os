#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

void print(const char *str);
void terminal_clear();
void terminal_write_char(char c, uint8_t forecolor, uint8_t backcolor);
void kprintf(char *fmt, ...);

void ksprintf(const char *str, int max);

#endif