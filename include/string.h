#pragma once

#include <pci.h>
#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int strnlen_terminator(const char *s, size_t maxlen, char terminator);
int memcmp(const void *v1, const void *v2, unsigned int n);
__attribute__((nonnull)) int strncmp(const char *p, const char *q, unsigned int n);
__attribute__((nonnull)) char *strncpy(char *dest, const char *src, size_t n);
bool isdigit(char c);
int tonumericdigit(char c);
bool isspace(char c);
char *trim(char str[static 1], size_t max);
char *substring(const char str[static 1], int start, int end);
int istrncmp(const char s1[static 1], const char s2[static 1], int n);
char tolower(char s1);
int itoa(int n, char s[static 1]);
int itohex(uint32_t n, char s[static 1]);
void reverse(char s[static 1]);
/* Duplicate S, returning an identical malloc'd string.  */
char *strdup(const char s[static 1]);
// char *strtok(char *str, const char delim[static 1]);
char *strtok(char *s, const char delim[static 1]);
char *strcat(char dest[static 1], const char src[static 1]);
char *strncat(char *dest, const char *src, size_t n);
int count_words(const char input[static 1]);
char *strchr(const char *s, int c);
uint32_t atoi(const char str[static 1]);
bool starts_with(const char pre[static 1], const char str[static 1]);
bool str_ends_with(const char *str, const char *suffix);
