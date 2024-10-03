#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

// #define KDEBUG_SERIAL

#ifdef KDEBUG_SERIAL
#define dbgprintf(a, ...) serial_printf("%s(): " a, __func__, ##__VA_ARGS__)
#else
#define dbgprintf(a, ...)
#endif

#ifdef KDEBUG_WARNINGS
#define warningf(a, ...) serial_printf("WARNING: %s(): " a, __func__, ##__VA_ARGS__)
#else
#define warningf(a, ...)
#endif

void init_serial();
int32_t serial_printf(char *fmt, ...);
void serial_put(char a);
#endif
