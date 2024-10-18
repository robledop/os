#include "ps2_kbd.h"
#include "console.h"
#include "idt.h"
#include "io.h"
#include "kernel.h"
#include "keyboard.h"
#include "vga_buffer.h"

int ps2_keyboard_init();
void ps2_keyboard_interrupt_handler(int interrupt);

struct keyboard ps2_keyboard = {
    .name = {"ps2"},
    .init = ps2_keyboard_init,
};

#define PS2_CAPSLOCK 0x3A

void kbd_ack(void) {
    while (inb(0x60) != 0xfa)
        ;
}

void kbd_led_handling(unsigned char ledstatus) {
    outb(0x60, 0xed);
    kbd_ack();
    outb(0x60, ledstatus);
}

// Taken from xv6
uchar keyboard_get_char() {
    // static bool ctrl_pressed = false;
    // static bool key_released = false;
    static unsigned int shift;
    static uchar *charcode[4] = {normalmap, shiftmap, ctlmap, ctlmap};

    const unsigned int st = inb(KBD_STATUS_PORT);
    if ((st & KBD_DATA_IN_BUFFER) == 0) {
        return -1;
    }
    unsigned int data = inb(KBD_DATA_PORT);

    // // // Update Ctrl key state
    // if (data == 0x1D) { // Ctrl key
    //     key_released = false;
    //     ctrl_pressed = true;
    // }
    //
    // // Check for Function keys (F1, F2, F3)
    // if (!key_released && ctrl_pressed) {
    //     if (data == 0x3B) { // F1
    //         switch_console(0);
    //     } else if (data == 0x3C) { // F2
    //         switch_console(1);
    //     } else if (data == 0x3D) { // F3
    //         switch_console(2);
    //     }
    // }

    if (data == 0xE0) {
        shift |= E0ESC;
        return 0;
    }
    if (data & 0x80) {
        // Key released
        // key_released = true;
        data         = (shift & E0ESC ? data : data & 0x7F);
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
        if ('a' <= c && c <= 'z')
            c += 'A' - 'a';
        else if ('A' <= c && c <= 'Z')
            c += 'a' - 'A';
    }
    return c;
}

int ps2_keyboard_init() {
    idt_register_interrupt_callback(ISR_KEYBOARD, ps2_keyboard_interrupt_handler);

    outb(KBD_STATUS_PORT, 0xAE); // keyboard enable command
    outb(KBD_DATA_PORT, 0xFF); // keyboard reset command

    kbd_led_handling(0x07);

    return 0;
}

void ps2_keyboard_interrupt_handler(int interrupt) {
    kernel_page();

    const uchar c = keyboard_get_char();
    if (c > 0) {
        keyboard_push(c);
    }
    task_page();
}

struct keyboard *ps2_init() {
    return &ps2_keyboard;
}

// void keyboard_interrupt_handler() {
//     uint8_t scancode = read_scancode();
//     if (is_console_switch_key(scancode)) {
//         int console_number = get_console_number_from_scancode(scancode);
//         switch_console(console_number);
//     } else {
//         Console *console = &consoles[active_console];
//         char key = translate_scancode(scancode);
//         if (key) {
//             console->input_buffer[console->input_buffer_tail++] = key;
//             console->input_buffer_tail %= INPUT_BUFFER_SIZE;
//         }
//     }
// }
