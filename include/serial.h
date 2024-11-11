#pragma once


// #define DEBUG_SERIAL
#define DEBUG_WARNINGS

#ifdef DEBUG_SERIAL
#define dbgprintf(a, ...) serial_printf("%s(): " a, __func__, ##__VA_ARGS__)
#else
#define dbgprintf(a, ...)
#endif

#ifdef DEBUG_WARNINGS
#define warningf(a, ...) serial_printf("WARNING: file: %s, line: %d: \n" a, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define warningf(a, ...)
#endif

void init_serial();
int serial_printf(const char fmt[static 1], ...);
void serial_put(char a);