#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif


#define KEYBOARD_CAPSLOCK_ON 0x01
#define KEYBOARD_CAPSLOCK_OFF 0x00

typedef int KEYBOARD_CAPSLOCK_STATE;

typedef int (*KEYBOARD_INIT_FUNCTION)();
struct keyboard {
    KEYBOARD_INIT_FUNCTION init;
    char name[20];
    struct keyboard *next;
};

void keyboard_init(void);
// __attribute__((nonnull)) void keyboard_backspace(struct process *process);
void keyboard_push(uint8_t c);
uint8_t keyboard_pop(void);
__attribute__((nonnull)) int keyboard_register(const struct keyboard *kbd);
