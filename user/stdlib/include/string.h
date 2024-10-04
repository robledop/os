#ifndef STDLIBSTRING_H
#define STDLIBSTRING_H

#include <stdbool.h>
#include <stddef.h>

int strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
int strnlen_terminator(const char *s, size_t maxlen, char terminator);
int strncmp(const char *p, const char *q, unsigned int n);
char *strncpy(char *dest, const char *src, int n);
char *safestrcpy(char *s, const char *t, int n);
bool isdigit(char c);
int tonumericdigit(char c);
bool isspace(char c);
char *trim(char *str);
char *substring(char *str, int start, int end);
int istrncmp(const char *s1, const char *s2, int n);
char tolower(char s1);
int itohex(unsigned int n, char s[]);
void reverse(char s[]);
char *itoa(int i);
char *strtok(char *str, const char *delimiters);

#endif