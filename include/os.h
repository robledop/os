#pragma once

#include "stdio.h"


struct command_argument *os_parse_command(const char command[static 1], int max);
void os_terminal_readline(unsigned char out[static 1], int max, bool output_while_typing);
