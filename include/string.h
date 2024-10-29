#pragma once

#include "types.h"

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int strnlen_terminator(const char *s, size_t maxlen, char terminator);
int memcmp(const void *v1, const void *v2, unsigned int n);
int strncmp(const char *p, const char *q, unsigned int n);
char *strncpy(char *dest, const char *src, size_t n);
char *safestrcpy(char *s, const char *t, int n);
bool isdigit(char c);
int tonumericdigit(char c);
bool isspace(char c);
char *trim(char *str);
char *substring(const char *str, const int start, const int end);

int istrncmp(const char *s1, const char *s2, int n);
char tolower(char s1);

int itoa(int n, char s[]);
int itohex(uint32_t n, char s[]);
void reverse(char s[]);
/* Duplicate S, returning an identical malloc'd string.  */
char *strdup(const char *s);
char *strtok(char *str, const char *delim);

char *strcat(char *dest, const char *src);
int count_words(const char *input);


char *strcpy(char *dest, const char *src);
uint32_t atoi(const char *str);
bool starts_with(const char *pre, const char *str);
bool str_ends_with(const char *str, const char *suffix);
