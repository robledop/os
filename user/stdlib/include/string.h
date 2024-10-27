#pragma once
#include "types.h"

int strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int strnlen_terminator(const char *s, size_t maxlen, char terminator);
int strncmp(const char *p, const char *q, unsigned int n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, int n);
char *safestrcpy(char *s, const char *t, int n);
bool isdigit(char c);
int tonumericdigit(char c);
bool isspace(char c);
char *trim(char *str);
char *substring(const char *str, int start, int end);
int istrncmp(const char *s1, const char *s2, int n);
char tolower(char s1);
int itohex(unsigned int n, char s[]);
void reverse(char s[]);
char *itoa(int i);
uint32_t atoi(const char *str);
char *strtok(char *str, const char *delimiters);
bool starts_with(const char *pre, const char *str);
char *strcat(char *dest, const char *src);
bool str_ends_with(const char *str, const char *suffix);
char *strdup(const char *s);
char *strcat(char *dest, const char *src);
int count_words(const char *input);
