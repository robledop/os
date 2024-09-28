#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *str);
char *int_to_string(int num);
char *hex_to_string(uint32_t num);

#endif