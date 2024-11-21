#include <memory.h>
#include <string.h>
#ifdef __KERNEL__
#include <kernel.h>
#include <kernel_heap.h>
#else
#include <stdlib.h>
#endif

// This file is also included in the stdlib

inline void reverse(char s[static 1])
{
    int i, j;

    for (i = 0, j = (int)strlen(s) - 1; i < j; i++, j--) {
        const int c = (uint8_t)s[i];
        s[i]        = s[j];
        s[j]        = c;
    }
}

// Gets the length of a string
size_t strlen(const char *s)
{
    if (s == NULL) {
        return 0;
    }
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

// Gets the length of a string
size_t strnlen(const char *s, const size_t maxlen)
{
    size_t n;

    for (n = 0; s[n]; n++) {
        if (n == maxlen) {
            break;
        }
    }
    return n;
}

int strnlen_terminator(const char *s, const size_t maxlen, const char terminator)
{
    size_t n;

    for (n = 0; s[n]; n++) {
        if (n == maxlen || s[n] == terminator || s[n] == '\0') {
            break;
        }
    }
    return (int)n;
}

// Compare two memory blocks
int memcmp(const void *v1, const void *v2, unsigned int n)
{
    const unsigned char *s1 = v1;
    const unsigned char *s2 = v2;
    while (n-- > 0) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++, s2++;
    }

    return 0;
}

// Compare two strings
int strncmp(const char *p, const char *q, unsigned int n)
{
    while (n > 0 && *p && *p == *q) {
        n--, p++, q++;
    }
    if (n == 0) {
        return 0;
    }
    return (unsigned char)*p - (unsigned char)*q;
}

char tolower(char s1)
{
    if (s1 >= 65 && s1 <= 90) {
        s1 += 32;
    }

    return s1;
}

// Compare two strings ignoring case
int istrncmp(const char s1[static 1], const char s2[static 1], int n)
{
    while (n-- > 0) {
        const uint8_t u1 = (unsigned char)*s1++;
        const uint8_t u2 = (unsigned char)*s2++;
        if (u1 != u2 && tolower(u1) != tolower(u2)) {
            return u1 - u2;
        }
        if (u1 == '\0') {
            return 0;
        }
    }

    return 0;
}

// Copy string t to s
char *strncpy(char *dest, const char *src, const size_t n)
{
    size_t i = 0;
    for (i = 0; i < n - 1; i++) {
        if (src[i] == '\0') {
            break;
        }
        dest[i] = src[i];
    }

    dest[i] = '\0';

    return dest;
}

bool isdigit(char c)
{
    return c >= '0' && c <= '9';
}

int tonumericdigit(char c)
{
    return c - 48;
}

bool isspace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

// Trim leading and trailing whitespaces
char *trim(char str[static 1], size_t max)
{
    size_t i = 0;
    while (isspace(*str) && i < max) {
        str++;
        i++;
    }
    if (*str == 0) {
        return str;
    }
    char *end = str + strnlen(str, max - i) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }
    end[1] = '\0';
    return str;
}

char *substring(const char str[static 1], const int start, const int end)
{
#ifdef __KERNEL__
    auto const substr = (char *)kmalloc(sizeof(char) * (end - start + 2));
#else
    auto const substr = (char *)malloc(sizeof(char) * (end - start + 2));
#endif

    for (int i = start; i <= end; i++) {
        substr[i - start] = str[i];
    }
    substr[end - start + 1] = '\0';
    return substr;
}

// void substring(const char str[static 1], const int start, const int end, char *buffer)
// {
//     for (int i = start; i <= end; i++) {
//         buffer[i - start] = str[i];
//     }
//     buffer[end - start + 1] = '\0';
// }

inline int itoa(int n, char s[static 1])
{
    int sign;

    if ((sign = n) < 0) {
        n = -n;
    }
    int i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) {
        s[i++] = '-';
    }

    s[i] = '\0';
    reverse(s);

    return i;
}

uint32_t atoi(const char str[static 1])
{
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

inline int itohex(uint32_t n, char s[static 1])
{
    int i = 0;
    do {
        const uint32_t d = n % 16;
        if (d < 10) {
            s[i++] = d + '0';
        } else {
            s[i++] = d - 10 + 'a';
        }
    } while ((n /= 16) > 0);
    s[i] = 0;
    reverse(s);

    return i;
}

char *strchr(const char *s, int c)
{
    while (*s != '\0') {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    return NULL;
}

char *strtok(char *str, const char delim[static 1])
{
    static char *next = NULL;
    // If str is provided, start from the beginning
    if (str != NULL) {
        next = str;
    } else {
        // If no more tokens, return NULL
        if (next == NULL) {
            return NULL;
        }
    }

    // Skip leading delimiters
    while (*next != '\0' && strchr(delim, *next) != NULL) {
        next++;
    }

    // If end of string reached after skipping delimiters
    if (*next == '\0') {
        next = NULL;
        return NULL;
    }

    // Mark the start of the token
    char *start = next;

    // Find the end of the token
    while (*next != '\0' && strchr(delim, *next) == NULL) {
        next++;
    }

    // If end of token is not the end of the string, terminate it
    if (*next != '\0') {
        *next = '\0';
        next++; // Move past the null terminator
    } else {
        // No more tokens
        next = NULL;
    }

    return start;
}

/// @brief Duplicate s, returning an identical malloc'd string.
char *strdup(const char s[static 1])
{
    const size_t len = strlen(s);
#ifdef __KERNEL__
    char *new = kmalloc(len + 1);
#else
    char *new = malloc(len + 1);
#endif


    if (new == NULL) {
        return nullptr;
    }
    strncpy(new, s, len + 1);
    return new;
}

/// @brief Concatenate two strings
/// @param dest the destination string
/// @param src the source string
/// @return the concatenated string
char *strcat(char dest[static 1], const char src[static 1])
{
    char *d = dest;
    while (*d != '\0') {
        d++;
    }
    while (*src != '\0') {
        *d = *src;
        d++;
        src++;
    }
    *d = '\0';
    return dest;
}

char *strncat(char *dest, const char *src, size_t n)
{
    char *d = dest;

    while (*d != '\0') {
        d++;
    }

    // Append up to 'n' characters from 'src' to 'dest'
    while (n-- > 0 && *src != '\0') {
        *d++ = *src++;
    }

    *d = '\0';

    return dest;
}

int count_words(const char input[static 1])
{
    int count         = 0;
    char *temp        = strdup(input);
    const char *token = strtok(temp, " ");
    while (token != nullptr) {
        count++;
        token = strtok(nullptr, " ");
    }
#ifdef __KERNEL__
    kfree(temp);
#else
    free(temp);
#endif

    return count;
}

bool starts_with(const char pre[static 1], const char str[static 1])
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

/// @brief Check if a string ends with a suffix
/// @param str string to check
/// @param suffix suffix to check for
/// @return true if the string ends with the suffix, false otherwise
bool str_ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix) {
        return false;
    }

    const size_t str_len    = strlen(str);
    const size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}
