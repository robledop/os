#include "terminal.h"
#include "config.h"
#include "io.h"
#include "memory.h"
#include "serial.h"
#include "string.h"

#define va_start(v, l) __builtin_va_start(v, l)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_copy(d, s) __builtin_va_copy(d, s)

typedef __builtin_va_list va_list;

static uint8_t forecolor = 0x0F; // Default white
static uint8_t backcolor = 0x00; // Default black

void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void write_character(unsigned char c, unsigned char forecolour, unsigned char backcolour, int x, int y) {
    uint16_t attrib = (backcolour << 4) | (forecolour & 0x0F);
    volatile uint16_t *where;
    where  = (volatile uint16_t *)0xB8000 + (y * 80 + x);
    *where = c | (attrib << 8);
}

void terminal_write_char(char c, uint8_t forecolor, uint8_t backcolor) {
    static int x = 0;
    static int y = 0;
    switch (c) {
    case 0x08: // Backspace
        if (x > 0) {
            x--;
            write_character(' ', forecolor, backcolor, x, y);
            if (x == 0 && y > 0) {
                x = VGA_WIDTH - 1;
                y--;
            }
        }
        break;
    case '\n': // Newline
    case '\r':
        x = 0;
        y++;
        break;
    case '\t': // Tab
        x += 4;
        if (x >= VGA_WIDTH) {
            x = 0;
            y++;
        }
        break;
    default:
        write_character(c, forecolor, backcolor, x, y);
        x++;
        if (x >= VGA_WIDTH) {
            x = 0;
            y++;
        }
    }

    update_cursor(x, y);
}

void print(const char *str) {
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) {
        terminal_write_char(str[i], forecolor, backcolor);
    }
}

void ksprint(const char *str, int max) {
    size_t len = strnlen(str, max);
    for (int i = 0; i < len; i++) {
        terminal_write_char(str[i], 0x0F, 0x00);
    }
}

void kprint(char *fmt, ...) {
    va_list args;

    int x_offset = 0;
    char str[MAX_FMT_STR];
    int num = 0;

    va_start(args, fmt);

    while (*fmt != '\0') {
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

            case 'x':
                num = va_arg(args, int);
                itohex(num, str);
                print("0x");
                print(str);
                x_offset += strlen(str);
                break;

            case 's':
                char *str_arg = va_arg(args, char *);
                print(str_arg);
                x_offset += strlen(str_arg);
                break;

            case 'c':
                char char_arg = (char)va_arg(args, int);
                terminal_write_char(char_arg, forecolor, backcolor);
                x_offset++;
                break;

            default:
                break;
            }
            fmt++;
            break;
        case '\033':
            // Handle ANSI escape sequences
            fmt++;
            if (*fmt != '[') {
                break;
            }
            fmt++;
            int param1 = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                param1 = param1 * 10 + (*fmt - '0');
                fmt++;
            }
            if (*fmt == ';') {
                fmt++;
                int param2 = 0;
                while (*fmt >= '0' && *fmt <= '9') {
                    param2 = param2 * 10 + (*fmt - '0');
                    fmt++;
                }
                if (*fmt == 'm') {
                    switch (param1) {
                    case 30:
                        forecolor = 0x00;
                        break; // Black
                    case 31:
                        forecolor = 0x04;
                        break; // Red
                    case 32:
                        forecolor = 0x02;
                        break; // Green
                    case 33:
                        forecolor = 0x0E;
                        break; // Yellow
                    case 34:
                        forecolor = 0x01;
                        break; // Blue
                    case 35:
                        forecolor = 0x05;
                        break; // Magenta
                    case 36:
                        forecolor = 0x03;
                        break; // Cyan
                    case 37:
                        forecolor = 0x0F;
                        break; // White
                    default:
                        break;
                    }
                    switch (param2) {
                    case 40:
                        backcolor = 0x00;
                        break; // Black
                    case 41:
                        backcolor = 0x04;
                        break; // Red
                    case 42:
                        backcolor = 0x02;
                        break; // Green
                    case 43:
                        backcolor = 0x0E;
                        break; // Yellow
                    case 44:
                        backcolor = 0x01;
                        break; // Blue
                    case 45:
                        backcolor = 0x05;
                        break; // Magenta
                    case 46:
                        backcolor = 0x03;
                        break; // Cyan
                    case 47:
                        backcolor = 0x0F;
                        break; // White
                    default:
                        break;
                    }
                    // Apply the colors to the next characters
                    terminal_write_char(' ', forecolor, backcolor);
                }
            } else if (*fmt == 'm') {
                switch (param1) {
                case 30:
                    forecolor = 0x00;
                    break; // Black
                case 31:
                    forecolor = 0x04;
                    break; // Red
                case 32:
                    forecolor = 0x02;
                    break; // Green
                case 33:
                    forecolor = 0x0E;
                    break; // Yellow
                case 34:
                    forecolor = 0x01;
                    break; // Blue
                case 35:
                    forecolor = 0x05;
                    break; // Magenta
                case 36:
                    forecolor = 0x03;
                    break; // Cyan
                case 37:
                    forecolor = 0x0F;
                    break; // White
                default:
                    break;
                }
            }
            break;

        default:
            terminal_write_char(*fmt, forecolor, backcolor);
        }
        fmt++;
    }
}

void terminal_clear() {
    dbgprintf("Clearing terminal\n");

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            write_character(' ', forecolor, backcolor, x, y);
        }
    }
}