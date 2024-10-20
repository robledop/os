#pragma once

#include "multiboot.h"

void kernel_main(multiboot_info_t *mbd, unsigned int magic);
void panic(const char *msg);
void kernel_page();
void set_kernel_mode_segments();
void start_shell(int console);
void system_reboot();
void system_shutdown();

#define ERROR(x) ((void *)(x))
#define ERROR_I(x) ((int)(x))
#define ISERR(x) ((int)(x) < 0)
