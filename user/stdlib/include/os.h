#pragma once

#include "stdio.h"
#include "types.h"

#define MAX_PATH_LENGTH 1024

struct command_argument *os_parse_command(const char *command, int max);
void os_terminal_readline(unsigned char *out, int max, bool output_while_typing);
void os_putchar_color(char c, unsigned char forecolor, unsigned char backcolor);
int os_create_process(const char *command, const char *current_directory);
