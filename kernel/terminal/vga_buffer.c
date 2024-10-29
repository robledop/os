#include "vga_buffer.h"

#include <spinlock.h>
#include <stdarg.h>
#include "config.h"
#include "console.h"
#include "debug.h"
#include "io.h"
#include "memory.h"
#include "string.h"

// extern int active_console;
// extern Console consoles[NUM_CONSOLES];

#define BYTES_PER_CHAR 2 // 1 byte for character, 1 byte for attribute (color)
#define SCREEN_SIZE (VGA_WIDTH * VGA_HEIGHT * BYTES_PER_CHAR)
#define ROW_SIZE (VGA_WIDTH * BYTES_PER_CHAR)
// #define DEFAULT_FOREGROUND 0x07
// #define DEFAULT_BACKGROUND 0x00

// uint8_t forecolor = 0x0F; // Default white
// uint8_t backcolor = 0x00; // Default cyan

uint8_t attribute = DEFAULT_ATTRIBUTE;

static int cursor_y;
static int cursor_x;
// int blinking = 0;
// int bold     = 0;

int ansi_to_vga_foreground[] = {
    0x00, // Black
    0x04, // Red
    0x02, // Green
    0x0E, // Yellow (Brown in VGA)
    0x01, // Blue
    0x05, // Magenta
    0x03, // Cyan
    0x07  // White (Light Grey in VGA)
};

int ansi_to_vga_background[] = {
    0x00, // Black
    0x40, // Red
    0x20, // Green
    0xE0, // Yellow (Brown in VGA)
    0x10, // Blue
    0x50, // Magenta
    0x30, // Cyan
    0x70  // White (Light Grey in VGA)
};

spinlock_t vga_lock;


void enable_cursor(const uint8_t cursor_start, const uint8_t cursor_end)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void update_cursor(const int row, const int col)
{
    const uint16_t position = (row * VGA_WIDTH) + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (position >> 8) & 0xFF);
}

void vga_buffer_write(const char c, const uint8_t attribute, const int x, const int y)
{
    const int pos_x = x == -1 ? cursor_x : x;
    const int pos_y = y == -1 ? cursor_y : y;

    volatile uint16_t *where = (volatile uint16_t *)VIDEO_MEMORY + (pos_y * VGA_WIDTH + pos_x);
    *where                   = c | (attribute << 8);
}

static void write_character(const uchar c, const int x, const int y)
{
    ASSERT(x < VGA_WIDTH, "X is out of bounds");
    ASSERT(y < VGA_HEIGHT, "Y is out of bounds");
    // ASSERT(forecolor != 0x00, "Foreground color is black");

    // Left arrow key
    if (c == 228) {
        cursor_x -= 2;
        update_cursor(cursor_y, cursor_x);
        return;
    }

    // Right arrow key
    if (c == 229) {
        update_cursor(cursor_y, cursor_x);
        return;
    }

    // CTRL + F1
    if (c == 232) {
        // Switch to terminal 1
        return;
    }

    // CTRL + F2
    if (c == 233) {
        // Switch to terminal 2
        return;
    }

    vga_buffer_write(c, attribute, x, y);
}


uint16_t get_cursor_position(void)
{
    uint16_t pos = 0;
    outb(0x3D4, 0x0F);
    pos |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;
    return pos;
}

void scroll_screen()
{
    auto const video_memory = (uchar *)VIDEO_MEMORY;
    // auto const video_memory = (uchar *)consoles[active_console].framebuffer;

    // Move all rows up by one
    memmove(video_memory, video_memory + ROW_SIZE, SCREEN_SIZE - ROW_SIZE);

    // Clear the last line (fill with spaces and default attribute)
    for (size_t i = SCREEN_SIZE - ROW_SIZE; i < SCREEN_SIZE; i += BYTES_PER_CHAR) {
        video_memory[i]     = ' ';
        video_memory[i + 1] = DEFAULT_ATTRIBUTE;
    }

    if (cursor_y > 0) {
        cursor_y--;
    }
    update_cursor(cursor_y, cursor_x);
}

void terminal_putchar(const char c, const uint8_t attr, const int x, const int y)
{
    attribute = attr == 0 ? DEFAULT_ATTRIBUTE : attr;

    switch (c) {
    case 0x08: // Backspace
        if (cursor_x > 0) {
            cursor_x--;
            if (cursor_x == 0 && cursor_y > 0) {
                cursor_x = VGA_WIDTH - 1;
                cursor_y--;
            }
            write_character(' ', cursor_x, cursor_y);
        }
        break;
    case '\n': // Newline
    case '\r':
        cursor_x = 0;
        cursor_y++;
        break;
    case '\t': // Tab
        cursor_x += 4;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
        break;
    default:
        write_character(c, -1, -1);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    // Scroll if needed
    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen();
        cursor_y = VGA_HEIGHT - 1;
    }

    update_cursor(cursor_y, cursor_x);
}

void print(const char *str)
{
    const size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        terminal_putchar(str[i], attribute, -1, -1);
    }
}

// BUG: There seems to be a bug that prevents more than 3 arguments from being printed
void kprintf(const char *fmt, ...)
{
    // ASSERT(forecolor != 0x00, "Foreground color is black");

    va_list args;

    size_t x_offset = 0;
    int num         = 0;

    va_start(args, fmt);

    while (*fmt != '\0') {
        char str[MAX_FMT_STR];
        switch (*fmt) {
        case '%':
            memset(str, 0, MAX_FMT_STR);
            switch (*(fmt + 1)) {
            case 'd':
                num = va_arg(args, int);
                itoa(num, str);
                print(str);
                x_offset += strlen(str);
                break;

            case 'p':
            case 'x':
                num = va_arg(args, int);
                itohex(num, str);
                print("0x");
                print(str);
                x_offset += strlen(str);
                break;

            case 's':
                const char *str_arg = va_arg(args, char *);
                print(str_arg);
                x_offset += strlen(str_arg);
                break;

            case 'c':
                const char char_arg = (char)va_arg(args, int);
                terminal_putchar(char_arg, attribute, -1, -1);
                // terminal_write_char(char_arg, forecolor, backcolor);
                x_offset++;
                break;

            default:
                break;
            }
            fmt++;
            break;
        case '\033':
            {
                // Handle ANSI escape sequences
                fmt++;
                if (*fmt != '[') {
                    break;
                }
                fmt++;
                int params[10];
                int param_count = 0;
                while (true) {
                    int param = 0;
                    while (*fmt >= '0' && *fmt <= '9') {
                        param = param * 10 + (*fmt - '0');
                        fmt++;
                    }
                    params[param_count++] = param;
                    if (*fmt == ';') {
                        fmt++;
                    } else {
                        break;
                    }
                }
                static int blinking = 0;
                static bool bold    = false;

                if (*fmt == 'm') {
                    for (int i = 0; i < param_count; i++) {
                        switch (params[i]) {
                        case 0:
                            attribute = DEFAULT_ATTRIBUTE;
                            blinking  = 0;
                            bold      = false;
                            break;
                        case 1:
                            bold = true;
                            break;
                        case 5:
                            blinking = 1;
                            break;
                        case 22:
                            bold = false;
                            break;
                        case 25:
                            blinking = 0;
                            break;

                        default:
                            {
                                int forecolor = 0x07;
                                int backcolor = 0x00;
                                if (params[i] >= 30 && params[i] <= 37) {
                                    const int color_index = params[i] - 30;
                                    forecolor             = ansi_to_vga_foreground[color_index];
                                    if (bold) {
                                        forecolor |= 0x08; // Set intensity bit for bold text
                                    }
                                } else if (params[i] >= 40 && params[i] <= 47) {
                                    const int color_index = params[i] - 40;
                                    backcolor             = ansi_to_vga_foreground[color_index]; // Use the same mapping
                                }

                                attribute = ((blinking & 1) << 7) | ((backcolor & 0x07) << 4) | (forecolor & 0x0F);
                            }
                            break;
                        }
                    }
                }
            }
            break;
        default:
            terminal_putchar(*fmt, attribute, -1, -1);
        }
        fmt++;
    }

    va_end(args);
}

void terminal_clear()
{
    // spin_lock(&vga_lock);
    cursor_x = 0;
    cursor_y = 0;
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            write_character(' ', x, y);
        }
    }

    enable_cursor(14, 15);

    // spin_unlock(&vga_lock);
}

void vga_buffer_init()
{
    cursor_x = 0;
    cursor_y = 0;
    spinlock_init(&vga_lock);
    terminal_clear();
}