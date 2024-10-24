#include "keyboard.h"

#include <scheduler.h>

#include "assert.h"
#include "ps2_kbd.h"
#include "serial.h"
#include "status.h"
#include "task.h"
#include "vga_buffer.h"

static struct keyboard *keyboard_list_head = nullptr;
static struct keyboard *keyboard_list_tail = nullptr;

void keyboard_init()
{
    keyboard_register(ps2_init());
}

int keyboard_register(struct keyboard *kbd)
{
    if (kbd->init == NULL) {
        warningf("keyboard->init is NULL\n");
        ASSERT(false, "keyboard->init is NULL");
        return -EINVARG;
    }

    if (keyboard_list_tail) {
        keyboard_list_tail->next = kbd;
        keyboard_list_tail       = kbd;
    } else {
        keyboard_list_head = kbd;
        keyboard_list_tail = kbd;
    }

    return kbd->init();
}

static int keyboard_get_tail_index(const struct process *process)
{
    return process->keyboard.tail % KEYBOARD_BUFFER_SIZE;
}

void keyboard_backspace(struct process *process)
{
    process->keyboard.tail               = (process->keyboard.tail - 1) % KEYBOARD_BUFFER_SIZE;
    const int real_index                 = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = 0x00;
}

void keyboard_push(const uchar c)
{
    if (c == 0x00) {
        return;
    }

    struct process *process = scheduler_get_current_process();
    if (!process) {
        warningf("No current process\n");
        return;
    }

    const int real_index                 = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = c;
    process->keyboard.tail               = (process->keyboard.tail + 1) % KEYBOARD_BUFFER_SIZE;
}

uchar keyboard_pop()
{
    if (!scheduler_get_current_task()) {
        warningf("No current task\n");
        return 0;
    }

    struct process *process = scheduler_get_current_task()->process;

    const int real_index = process->keyboard.head % KEYBOARD_BUFFER_SIZE;
    const uchar c        = process->keyboard.buffer[real_index];

    if (c == 0x00) {
        return 0;
    }

    process->keyboard.buffer[real_index] = 0x00;
    process->keyboard.head               = (process->keyboard.head + 1) % KEYBOARD_BUFFER_SIZE;

    return c;
}
