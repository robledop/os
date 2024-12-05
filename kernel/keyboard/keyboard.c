#include <kernel.h>
#include <keyboard.h>
#include <ps2_kbd.h>
#include <status.h>
#include <tty.h>

void keyboard_init()
{
    keyboard_register(ps2_init());
}

int keyboard_register(const struct keyboard *kbd)
{
    if (kbd->init == nullptr) {
        panic("Keyboard init function is not available.");
        return -EINVARG;
    }

    return kbd->init();
}

void keyboard_push(const uint8_t c)
{
    if (c == 0x00) {
        return;
    }

    tty_input_buffer_put(c);
}