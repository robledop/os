#ifndef KERNEL_H
#define KERNEL_H
#include "multiboot.h"

#include <stdint.h>

void kernel_main(multiboot_info_t *mbd, unsigned int magic);
void panic(const char *msg);
void kernel_page();
void kernel_registers();

#define ERROR(x) ((void *)(x))
#define ERROR_I(x) ((int)(x))
#define ISERR(x) ((int)(x) < 0)

#endif