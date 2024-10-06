#include "terminal.h"
#include "config.h"
#include "io.h"
#include "memory.h"
#include "string.h"
#include <stdarg.h>
#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define BYTES_PER_CHAR 2
#define SCREEN_SIZE (VGA_WIDTH * VGA_HEIGHT * BYTES_PER_CHAR)
#define ROW_SIZE (VGA_WIDTH * BYTES_PER_CHAR)
#define DEFAULT_ATTRIBUTE 0x07 // Light grey on black background

uint8_t forecolor = 0x0F; // Default white
uint8_t backcolor = 0x00; // Default cyan

static int cursor_y;
static int cursor_x;

void update_cursor(int row, int col)
{
    unsigned short position = (row * VGA_WIDTH) + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

void write_character(unsigned char c, unsigned char fcolor, unsigned char bcolor, int x, int y)
{
    unsigned short int attrib = (bcolor << 4) | (fcolor & 0x0F);
    volatile unsigned short int *where;
    where = (volatile unsigned short int *)VIDEO_MEMORY + (y * VGA_WIDTH + x);
    *where = c | (attrib << 8);
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
    unsigned char *video_memory = (unsigned char *)VIDEO_MEMORY;

    // Move all rows up by one
    memmove(
        video_memory,
        video_memory + ROW_SIZE,
        SCREEN_SIZE - ROW_SIZE);

    // Clear the last line (fill with spaces and default attribute)
    for (size_t i = SCREEN_SIZE - ROW_SIZE; i < SCREEN_SIZE; i += BYTES_PER_CHAR)
    {
        video_memory[i] = ' ';
        video_memory[i + 1] = DEFAULT_ATTRIBUTE;
    }

    if (cursor_y > 0)
    {
        cursor_y--;
    }
    update_cursor(cursor_y, cursor_x);
}

void terminal_write_char(char c, uint8_t fcolor, uint8_t bcolor)
{
    forecolor = fcolor;
    backcolor = bcolor;

    switch (c)
    {
    case 0x08: // Backspace
        if (cursor_x > 0)
        {
            cursor_x--;
            write_character(' ', forecolor, backcolor, cursor_x, cursor_y);
            if (cursor_x == 0 && cursor_y > 0)
            {
                cursor_x = VGA_WIDTH - 1;
                cursor_y--;
            }
        }
        break;
    case '\n': // Newline
    case '\r':
        cursor_x = 0;
        cursor_y++;
        break;
    case '\t': // Tab
        cursor_x += 4;
        if (cursor_x >= VGA_WIDTH)
        {
            cursor_x = 0;
            cursor_y++;
        }
        break;
    default:
        write_character(c, forecolor, backcolor, cursor_x, cursor_y);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH)
        {
            cursor_x = 0;
            cursor_y++;
        }
    }

    // Scroll if needed
    if (cursor_y >= VGA_HEIGHT)
    {
        scroll_screen();
        cursor_y = VGA_HEIGHT - 1;
    }

    update_cursor(cursor_y, cursor_x);
}

void print(const char *str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_write_char(str[i], forecolor, backcolor);
    }
}

void ksprintf(const char *str, int max)
{
    size_t len = strnlen(str, max);
    for (int i = 0; i < len; i++)
    {
        terminal_write_char(str[i], forecolor, backcolor);
    }
}

void kprintf(char *fmt, ...)
{
    va_list args;

    int x_offset = 0;
    char str[MAX_FMT_STR];
    int num = 0;

    va_start(args, fmt);

    while (*fmt != '\0')
    {
        switch (*fmt)
        {
        case '%':
            memset(str, 0, MAX_FMT_STR);
            switch (*(fmt + 1))
            {
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
            if (*fmt != '[')
            {
                break;
            }
            fmt++;
            int param1 = 0;
            while (*fmt >= '0' && *fmt <= '9')
            {
                param1 = param1 * 10 + (*fmt - '0');
                fmt++;
            }
            if (*fmt == ';')
            {
                fmt++;
                int param2 = 0;
                while (*fmt >= '0' && *fmt <= '9')
                {
                    param2 = param2 * 10 + (*fmt - '0');
                    fmt++;
                }
                if (*fmt == 'm')
                {
                    switch (param1)
                    {
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
                    switch (param2)
                    {
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
            }
            else if (*fmt == 'm')
            {
                switch (param1)
                {
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

    va_end(args);
}

void terminal_clear()
{
    cursor_x = 0;
    cursor_y = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            write_character(' ', forecolor, backcolor, x, y);
        }
    }
}