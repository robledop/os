#ifndef COMMAND_H
#define COMMAND_H

struct interrupt_frame;
void *isr80h_command0_sum(struct interrupt_frame *frame);
void *isr80h_command1_print(struct interrupt_frame *frame);
void* isr80h_command2_getkey(struct interrupt_frame *frame);
void* isr80h_command3_putchar(struct interrupt_frame *frame);
void* isr80h_command4_malloc(struct interrupt_frame *frame);
void* isr80h_command5_free(struct interrupt_frame *frame);
void* isr80h_command6_process_start(struct interrupt_frame *frame);
void *isr80h_command7_invoke_system(struct interrupt_frame *frame);
void *isr80h_command8_get_program_arguments(struct interrupt_frame *frame);

#endif