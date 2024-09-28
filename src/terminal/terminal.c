#include "terminal.h"
#include "string/string.h"
#include "config.h"

uint16_t *video_memory = 0;

uint16_t terminal_make_char(char c, uint8_t color)
{
    return (color << 8) | c;
}

void terminal_putchar(char c, uint8_t color, int x, int y)
{
    video_memory[y * VGA_WIDTH + x] = terminal_make_char(c, color);
}

void terminal_write_char(char c, uint8_t color)
{
    static int x = 0;
    static int y = 0;
    if (c == '\n')
    {
        x = 0;
        y++;
    }
    else
    {
        terminal_putchar(c, color, x, y);
        x++;
        if (x >= VGA_WIDTH)
        {
            x = 0;
            y++;
        }
    }
}

void print(const char *str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_write_char(str[i], 0x0F);
    }
}

void terminal_clear()
{
    video_memory = (uint16_t *)(0xB8000);
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(' ', 0x0F, x, y);
        }
    }
}