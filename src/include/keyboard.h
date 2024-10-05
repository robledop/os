#ifndef KEYBOARD_H
#define KEYBOARD_H
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
void keyboard_backspace(struct process* process);
void keyboard_push(char c);
char keyboard_pop();
int keyboard_insert(struct keyboard *kbd);

#endif