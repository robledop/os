#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include "process.h"

#define KEYBOARD_CAPSLOCK_ON 0x01
#define KEYBOARD_CAPSLOCK_OFF 0x00

typedef int KEYBOARD_CAPSLOCK_STATE;

typedef int (*KEYBOARD_INIT_FUNCTION)();
struct keyboard
{
    KEYBOARD_INIT_FUNCTION init;
    char name[20];
    struct keyboard *next;
};

void keyboard_init();
void keyboard_backspace(struct process *process);
void keyboard_push(const uchar c);
uchar keyboard_pop();
int keyboard_register(struct keyboard *kbd);
