#include "classic.h"
#include <stdint.h>
#include "idt.h"
#include "io.h"
#include "kernel.h"
#include "keyboard.h"

int classic_keyboard_init();
void classic_keyboard_interrupt_handler();

// https://wiki.osdev.org/PS/2_Keyboard#Scan_Code_Set_1
static uint8_t keyboard_scan_code_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08, '\t', 'Q',
    'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0x0d, 0x00, 'A', 'S', 'D', 'F',
    'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0x00, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',
    ',', '.', '/', 0x00, '*', 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'};

struct keyboard classic_keyboard = {
    .name = {"classic"},
    .init = classic_keyboard_init,
};

#define PS2_CAPSLOCK 0x3A

int classic_keyboard_init()
{
    idt_register_interrupt_callback(ISR_KEYBOARD, classic_keyboard_interrupt_handler);

    classic_keyboard.capslock_state = KEYBOARD_CAPSLOCK_OFF;

    // https://wiki.osdev.org/%228042%22_PS/2_Controller
    outb(PS2_PORT, PS2_COMMAND_ENABLE_PORT1);
    return 0;
}

uint8_t classic_keyboard_scan_code_to_ascii(uint8_t scan_code)
{
    if (scan_code > sizeof(keyboard_scan_code_set_one))
    {
        return 0;
    }
    char c = keyboard_scan_code_set_one[scan_code];

    if (classic_keyboard.capslock_state == KEYBOARD_CAPSLOCK_OFF && c >= 'A' && c <= 'Z')
    {
        c += 32;
    }

    return c;
}

void classic_keyboard_interrupt_handler()
{
    kernel_page();

    uint8_t scan_code = inb(KEYBOARD_INPUT_PORT);
    inb(KEYBOARD_INPUT_PORT); // ignore the second byte
    if (scan_code & PS2_KEYBOARD_KEY_RELEASED)
    {
        return;
    }

    if (scan_code == PS2_CAPSLOCK)
    {
        classic_keyboard.capslock_state = !classic_keyboard.capslock_state;
        return;
    }

    uint8_t c = classic_keyboard_scan_code_to_ascii(scan_code);
    if (c)
    {
        keyboard_push(c);
    }

    task_page();
}

struct keyboard *classic_init() { return &classic_keyboard; }