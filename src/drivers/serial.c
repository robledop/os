#include "serial.h"
#include "io.h"
#include "string.h"
#include "memory.h"
#include "kernel.h"
#include "task.h"
#include <stdarg.h>

#define PORT 0x3f8 // COM1
#define MAX_FMT_STR_SERIAL 50
static int serial_init_done = 0;

// extern int __cli_cnt;
int __cli_cnt = 0;

#define ENTER_CRITICAL() \
    __cli_cnt++;         \
    asm("cli");

#define LEAVE_CRITICAL() \
    __cli_cnt--;         \
    if (__cli_cnt == 0)  \
    {                    \
        asm("sti");      \
    }

void serial_put(char a)
{
    if (!serial_init_done)
    {
        return;
    }
    while ((inb(PORT + 5) & 0x20) == 0)
    {
    };

    outb(PORT, a);
}

void serial_write(char *str)
{
    if (!serial_init_done)
    {
        return;
    }
    for (size_t i = 0; i < strlen(str); i++)
    {
        serial_put(str[i]);
    }
}

int32_t serial_printf(char *fmt, ...)
{
    int written = 0;
#ifdef DEBUG_SERIAL
    ENTER_CRITICAL();
    va_list args;

    char str[MAX_FMT_STR_SERIAL];
    int num = 0;

    va_start(args, fmt);
    size_t i = 0;
    while (*fmt != '\0' && i++ < sizeof(str))
    {
        switch (*fmt)
        {
        case '%':
            memset(str, 0, sizeof(str));
            switch (*(fmt + 1))
            {
            case 'd':
                num = va_arg(args, int);
                itoa(num, str);
                serial_write(str);
                break;
            case 'x':
                num = va_arg(args, int);
                itohex(num, str);
                serial_write("0x");
                serial_write(str);
                break;
            case 's':;
                char *str_arg = va_arg(args, char *);
                serial_write(str_arg);
                break;
            case 'c':;
                char char_arg = (char)va_arg(args, int);
                serial_put(char_arg);
                break;

            default:
                break;
            }
            fmt++;
            break;
        default:
            serial_put(*fmt);
        }
        fmt++;
    }

    va_end(args);

#endif
    return written;
}

void init_serial()
{
#ifdef DEBUG_SERIAL

    // https://stackoverflow.com/questions/69481715/initialize-serial-port-with-x86-assembly
    outb(PORT + 1, 0x00); // Disable all interrupts
    outb(PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00); //                  (hi byte)
    outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
    outb(PORT + 4, 0x1E); // Set in loopback mode, test the serial chip
    outb(PORT + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (inb(PORT + 0) != 0xAE)
    {
        return;
    }

    outb(PORT + 4, 0x0F);

    serial_init_done = 1;
    serial_write("Serial debugging enabled!\n");
#endif
}
