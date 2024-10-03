#ifndef OS_H
#define OS_H

#include <stddef.h>
#include <stdbool.h>

struct command_argument
{
    char argument[512];
    struct command_argument *next;
};

struct process_arguments
{
    int argc;
    char **argv;
};

struct command_argument *os_parse_command(const char *command, int max);
void os_terminal_readline(char *out, int max, bool output_while_typing);
void os_process_start(const char *file_name);
int os_getkey_blocking();
void os_print(const char *str);
int os_getkey();
void *os_malloc(size_t size);
void os_free(void *ptr);
void os_putchar(char c);
void os_process_get_arguments(struct process_arguments *args);
int os_system(struct command_argument* args);
int os_system_run(const char *command);

#endif