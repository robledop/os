#include <idt.h>
#include <io.h>
#include <kernel_heap.h>
#include <keyboard.h>
#include <pic.h>
#include <ps2_kbd.h>
#include <scheduler.h>
#include <spinlock.h>
#include <string.h>

spinlock_t keyboard_lock         = 0;
spinlock_t keyboard_getchar_lock = 0;
int ps2_keyboard_init();
void ps2_keyboard_interrupt_handler(int interrupt, const struct interrupt_frame *frame);

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
    spin_lock(&keyboard_getchar_lock);

    static unsigned int shift;
    static uchar *charcode[4] = {normalmap, shiftmap, ctlmap, ctlmap};

    const unsigned int st = inb(KBD_STATUS_PORT);
    if ((st & KBD_DATA_IN_BUFFER) == 0) {
        spin_unlock(&keyboard_getchar_lock);
        return -1;
    }
    unsigned int data = inb(KBD_DATA_PORT);

    if (data == 0xE0) {
        shift |= E0ESC;
        spin_unlock(&keyboard_getchar_lock);
        return 0;
    }
    if (data & 0x80) {
        // Key released
        // key_released = true;
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        spin_unlock(&keyboard_getchar_lock);
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

    spin_unlock(&keyboard_getchar_lock);
    return c;
}

int ps2_keyboard_init()
{
    spinlock_init(&keyboard_lock);
    spinlock_init(&keyboard_getchar_lock);

    idt_register_interrupt_callback(ISR_KEYBOARD, ps2_keyboard_interrupt_handler);

    outb(KBD_STATUS_PORT, 0xAE); // keyboard enable command
    outb(KBD_DATA_PORT, 0xFF);   // keyboard reset command

    kbd_led_handling(0x07);

    return 0;
}

void ps2_keyboard_interrupt_handler(int interrupt, const struct interrupt_frame *frame)
{
    spin_lock(&keyboard_lock);

    pic_acknowledge(interrupt);

    const uchar c = keyboard_get_char();
    // Delete key
    if (c > 0 && c != 233) {
        keyboard_push(c);
    }

    auto const thread = scheduler_get_thread_sleeping_for_keyboard();
    if (thread) {
        thread->process->signal       = SIGWAKEUP;
        // thread->process->sleep_reason = SLEEP_REASON_NONE;
    }

    spin_unlock(&keyboard_lock);
}

struct keyboard *ps2_init()
{
    struct keyboard *kbd = kzalloc(sizeof(struct keyboard));
    strncpy(kbd->name, "ps2", sizeof(kbd->name));
    kbd->init = ps2_keyboard_init;


    return kbd;
}