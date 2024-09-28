#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int memcmp(const void *v1, const void *v2, unsigned int n);
int strncmp(const char *p, const char *q, unsigned int n);
char *strncpy(char *s, const char *t, int n);
char *safestrcpy(char *s, const char *t, int n);
char *hex_to_string(uint32_t num);
char *int_to_string(int num);
bool isdigit(char c);
int tonumericdigit(char c);

#endif