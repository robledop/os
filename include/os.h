#pragma once

#include "stdio.h"
#include "types.h"
#include <config.h>


struct command_argument *os_parse_command(const char *command, int max);
void os_terminal_readline(unsigned char *out, int max, bool output_while_typing);
