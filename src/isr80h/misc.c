#include "misc.h"
#include "idt.h"
#include "task.h"
#include "terminal.h"

void *isr80h_command0_sum(struct interrupt_frame *frame)
{
    int value2 = (int)task_get_stack_item(task_current(), 1);
    int value1 = (int)task_get_stack_item(task_current(), 0);
    return (void *)(value1 + value2);
}

void *isr80h_command1_print(struct interrupt_frame *frame)
{
    void *message = task_get_stack_item(task_current(), 0);
    char buffer[1024];

    copy_string_from_task(task_current(), message, buffer, sizeof(buffer));

    kprint(KYEL "Userland says: " KGRN "%s\n", buffer);

    return NULL;
}