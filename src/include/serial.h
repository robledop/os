#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include "terminal.h"

// #define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
// #define dbgprintf(a, ...) serial_printf("%s(): " a, __func__, ##__VA_ARGS__)
#define dbgprintf(a, ...) kprintf(a, ##__VA_ARGS__)
// #define dbgprintf(a, ...) serial_printf(a, ##__VA_ARGS__)
#else
#define dbgprintf(a, ...)
#endif

#ifdef DEBUG_WARNINGS
// #define warningf(a, ...) serial_printf("WARNING: %s(): " a, __func__, ##__VA_ARGS__)
#define warningf(a, ...) serial_printf("WARNING: " a, ##__VA_ARGS__)
#else
#define warningf(a, ...)
#endif

void init_serial();
int32_t serial_printf(char *fmt, ...);
void serial_put(char a);
#endif
