#pragma once

// https://wiki.osdev.org/8259_PIC

void pic_init(void);
void pic_acknowledge(int irq);
