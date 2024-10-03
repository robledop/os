#include "keyboard.h"
#include "classic.h"
#include "serial.h"
#include "status.h"
#include "task.h"
#include "terminal.h"

static struct keyboard *keyboard_list_head = NULL;
static struct keyboard *keyboard_list_tail = NULL;

void keyboard_init() { keyboard_insert(classic_init()); }

int keyboard_insert(struct keyboard *kbd)
{
    if (kbd->init == NULL)
    {
        dbgprintf("keyboard->init is NULL\n");
        return -EINVARG;
    }

    if (keyboard_list_tail)
    {
        keyboard_list_tail->next = kbd;
        keyboard_list_tail = kbd;
    }
    else
    {
        keyboard_list_head = kbd;
        keyboard_list_tail = kbd;
    }

    return kbd->init();
}

static int keyboard_get_tail_index(struct process *process) { return process->keyboard.tail % KEYBOARD_BUFFER_SIZE; }

void keyboard_backspace(struct process *process)
{
    process->keyboard.tail = (process->keyboard.tail - 1) % KEYBOARD_BUFFER_SIZE;
    int real_index = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = 0x00;
}

void keyboard_push(char c)
{
    if (c == 0x00)
    {
        return;
    }

    struct process *process = process_current();
    if (!process)
    {
        dbgprintf("No current process\n");
        return;
    }

    int real_index = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = c;
    // kprint("%c", c);
    dbgprintf("Pushed '%c' to keyboard buffer of process %d\n", c, process->pid);
    process->keyboard.tail = (process->keyboard.tail + 1) % KEYBOARD_BUFFER_SIZE;
    dbgprintf("Tail is now %d\n", process->keyboard.tail);
}

char keyboard_pop()
{
    if (!task_current())
    {
        dbgprintf("No current task\n");
        return 0;
    }

    struct process *process = task_current()->process;

    int real_index = process->keyboard.head % KEYBOARD_BUFFER_SIZE;
    char c = process->keyboard.buffer[real_index];

    if (c == 0x00)
    {
        dbgprintf("No character to pop\n");
        return 0;
    }

    process->keyboard.buffer[real_index] = 0x00;
    process->keyboard.head = (process->keyboard.head + 1) % KEYBOARD_BUFFER_SIZE;

    return c;
}
