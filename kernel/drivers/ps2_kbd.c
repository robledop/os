#include <idt.h>
#include <io.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <keyboard.h>
#include <ps2_kbd.h>
#include <scheduler.h>
#include <string.h>

int ps2_keyboard_init();
void ps2_keyboard_interrupt_handler(int interrupt, uint32_t unused);

#define PS2_CAPSLOCK 0x3A

void kbd_ack(void)
{
    while (inb(0x60) != 0xfa)
        ;
}

void kbd_led_handling(const unsigned char ledstatus)
{
    outb(0x60, 0xed);
    kbd_ack();
    outb(0x60, ledstatus);
}

// Taken from xv6
uchar keyboard_get_char()
{
    ENTER_CRITICAL();

    static unsigned int shift;
    static uchar *charcode[4] = {normalmap, shiftmap, ctlmap, ctlmap};

    const unsigned int st = inb(KBD_STATUS_PORT);
    if ((st & KBD_DATA_IN_BUFFER) == 0) {
        return -1;
    }
    unsigned int data = inb(KBD_DATA_PORT);

    if (data == 0xE0) {
        shift |= E0ESC;
        return 0;
    }
    if (data & 0x80) {
        // Key released
        // key_released = true;
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        return 0;
    }
    if (shift & E0ESC) {
        // Last character was an E0 escape; or with 0x80
        data |= 0x80;
        shift &= ~E0ESC;
    }

    shift |= shiftcode[data];
    shift ^= togglecode[data];
    uchar c = charcode[shift & (CTRL | SHIFT)][data];
    if (shift & CAPSLOCK) {
        if ('a' <= c && c <= 'z') {
            c += 'A' - 'a';
        } else if ('A' <= c && c <= 'Z') {
            c += 'a' - 'A';
        }
    }

    LEAVE_CRITICAL();
    return c;
}

int ps2_keyboard_init()
{
    idt_register_interrupt_callback(ISR_KEYBOARD, ps2_keyboard_interrupt_handler);

    outb(KBD_STATUS_PORT, 0xAE); // keyboard enable command
    outb(KBD_DATA_PORT, 0xFF);   // keyboard reset command

    kbd_led_handling(0x07);

    return 0;
}

void ps2_keyboard_interrupt_handler(int interrupt, uint32_t unused)
{
    ENTER_CRITICAL();

    const uchar c = keyboard_get_char();
    // Delete key
    if (c > 0 && c != 233) {
        keyboard_push(c);
    }
    if (scheduler_get_current_thread()->process->state == SLEEPING) {
        scheduler_get_current_thread()->process->signal = SIGWAKEUP;
    }
    LEAVE_CRITICAL();
}

struct keyboard *ps2_init()
{
    struct keyboard *kbd = kzalloc(sizeof(struct keyboard));
    strncpy(kbd->name, "ps2", sizeof(kbd->name));
    kbd->init = ps2_keyboard_init;


    return kbd;
}