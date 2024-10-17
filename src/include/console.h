#pragma once

#include "types.h"
#include "config.h"


#define BYTES_PER_CHAR 2
#define NUM_CONSOLES 3
#define FRAMEBUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT * BYTES_PER_CHAR)
#define INPUT_BUFFER_SIZE 256

typedef struct {
    uint8_t framebuffer[FRAMEBUFFER_SIZE];
    char input_buffer[INPUT_BUFFER_SIZE];
    int input_buffer_head;
    int input_buffer_tail;
} Console;

void initialize_consoles();
void switch_console(int console_number);
