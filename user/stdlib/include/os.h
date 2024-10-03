#ifndef OS_H
#define OS_H

#include <stddef.h>
#include <stdbool.h>

void os_terminal_readline(char *out, int max, bool output_while_typing);
void os_process_start(const char* file_name);
int os_getkey_blocking();
void os_print(const char *str);
int os_getkey();
void *os_malloc(size_t size);
void os_free(void *ptr);
void os_putchar(char c);

#endif