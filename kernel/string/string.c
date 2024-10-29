#include "string.h"
#include "types.h"
#ifdef __KERNEL__
#include "kernel_heap.h"
#else
#include <stdlib.h>
#endif

// This file is also included in the stdlib

inline void reverse(char s[])
{
    int i, j;

    for (i = 0, j = (int)strlen(s) - 1; i < j; i++, j--) {
        const int c = (uchar)s[i];
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
int istrncmp(const char *s1, const char *s2, int n)
{
    while (n-- > 0) {
        const uchar u1 = (unsigned char)*s1++;
        const uchar u2 = (unsigned char)*s2++;
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

// Like strncpy but guaranteed to NUL-terminate.
char *safestrcpy(char *s, const char *t, int n)
{
    char *os = s;
    if (n <= 0) {
        return os;
    }
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
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
char *trim(char *str)
{
    while (isspace(*str)) {
        str++;
    }
    if (*str == 0) {
        return str;
    }
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }
    end[1] = '\0';
    return str;
}

char *substring(const char *str, const int start, const int end)
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

inline int itoa(int n, char s[])
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

uint32_t atoi(const char *str)
{
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

inline int itohex(uint32_t n, char s[])
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

char *strtok(char *str, const char *delim)
{
    static char *static_str = nullptr; // Stores the string between calls
    int i = 0, j = 0;

    // If initial string is provided, reset static_str
    if (str != NULL) {
        static_str = str;
    } else {
        // If no more tokens, return NULL
        if (static_str == NULL) {
            return nullptr;
        }
    }

    // Skip leading delimiters
    while (static_str[i] != '\0') {
        for (j = 0; delim[j] != '\0'; j++) {
            if (static_str[i] == delim[j]) {
                break;
            }
        }
        if (delim[j] == '\0') {
            break;
        }
        i++;
    }

    // If end of string is reached, return NULL
    if (static_str[i] == '\0') {
        static_str = nullptr;
        return nullptr;
    }

    char *token = &static_str[i];

    // Find the end of the token
    while (static_str[i] != '\0') {
        for (j = 0; delim[j] != '\0'; j++) {
            if (static_str[i] == delim[j]) {
                static_str[i] = '\0'; // Terminate token
                i++;
                static_str += i; // Update static_str for next call
                return token;
            }
        }
        i++;
    }

    // No more delimiters; return the last token
    static_str = nullptr;
    return token;
}

/// @brief Duplicate s, returning an identical malloc'd string.
char *strdup(const char *s)
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
char *strcat(char *dest, const char *src)
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

int count_words(const char *input)
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

bool starts_with(const char *pre, const char *str)
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
