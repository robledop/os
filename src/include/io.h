#ifndef IO_H
#define IO_H

#include "types.h"

uint8_t inb(uint16_t p);
uint16_t inw(uint16_t p);
uint32_t inl(uint16_t p);
void outb(uint16_t portid, uint8_t value);
void outw(uint16_t portid, uint16_t value);
void outl(uint16_t portid, uint32_t value);
void cpu_print_info();
char *cpu_string();
int cpu_get_model(void);

#endif