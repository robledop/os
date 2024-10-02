#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

struct interrupt_frame;
void *isr80h_command0_sum(struct interrupt_frame *frame);
void *isr80h_command1_print(struct interrupt_frame *frame);
void* isr80h_command2_getkey(struct interrupt_frame *frame);
void* isr80h_command3_putchar(struct interrupt_frame *frame);

#endif