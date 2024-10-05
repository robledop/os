#include "string.h"
#include "stdlib.h"

/*
 * Functions from Kerninghan/Ritchie - The C Programming Language
 */
inline void reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

// Gets the length of a string
int strlen(const char *ptr)
{
    int i = 0;
    while (*ptr != 0)
    {
        i++;
        ptr += 1;
    }

    return i;
}

// Gets the length of a string
size_t strnlen(const char *s, size_t maxlen)
{
    int n;

    for (n = 0; s[n]; n++)
    {
        if (n == maxlen)
        {
            break;
        }
    }
    return n;
}

int strnlen_terminator(const char *s, size_t maxlen, char terminator)
{
    int n;

    for (n = 0; s[n]; n++)
    {
        if (n == maxlen || s[n] == terminator || s[n] == '\0')
        {
            break;
        }
    }
    return n;
}

// Compare two strings
int strncmp(const char *p, const char *q, unsigned int n)
{
    while (n > 0 && *p && *p == *q)
    {
        n--, p++, q++;
    }
    if (n == 0)
    {
        return 0;
    }
    return (unsigned char)*p - (unsigned char)*q;
}

bool starts_with(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

char tolower(char s1)
{
    if (s1 >= 65 && s1 <= 90)
    {
        s1 += 32;
    }

    return s1;
}

// Compare two strings ignoring case
int istrncmp(const char *s1, const char *s2, int n)
{
    unsigned char u1, u2;
    while (n-- > 0)
    {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if (u1 != u2 && tolower(u1) != tolower(u2))
            return u1 - u2;
        if (u1 == '\0')
            return 0;
    }

    return 0;
}

// Copy string t to s
char *strncpy(char *dest, const char *src, int n)
{
    int i = 0;
    for (i = 0; i < n - 1; i++)
    {
        if (src[i] == '\0')
        {
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
    char *os;

    os = s;
    if (n <= 0)
    {
        return os;
    }
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}

bool isdigit(char c) { return c >= '0' && c <= '9'; }

int tonumericdigit(char c) { return c - 48; }

bool isspace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'; }

// Trim leading and trailing whitespaces
char *trim(char *str)
{
    char *end;
    while (isspace(*str))
    {
        str++;
    }
    if (*str == 0)
    {
        return str;
    }
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
    {
        end--;
    }
    end[1] = '\0';
    return str;
}

char *substring(char *str, int start, int end)
{
    char *substr = (char *)malloc(sizeof(char) * (end - start + 2));
    for (int i = start; i <= end; i++)
    {
        substr[i - start] = str[i];
    }
    substr[end - start + 1] = '\0';
    return substr;
}

// inline int itoa(int n, char s[]) {
//     int i, sign;

//     if ((sign = n) < 0)
//         n = -n;
//     i = 0;
//     do {
//         s[i++] = n % 10 + '0';
//     } while ((n /= 10) > 0);

//     if (sign < 0)
//         s[i++] = '-';

//     s[i] = '\0';
//     reverse(s);

//     return i;
// }

// Convert integer to string
char *itoa(int i)
{
    static char str[12];
    int loc = 11;
    str[11] = 0;
    char neg = 1;
    if (i >= 0)
    {
        neg = 0;
        i = -i;
    }

    while (i)
    {
        str[--loc] = '0' - (i % 10);
        i /= 10;
    }

    if (loc == 11)
    {
        str[--loc] = '0';
    }

    if (neg)
    {
        str[--loc] = '-';
    }

    return &str[loc];
}

inline int itohex(unsigned int n, char s[])
{
    unsigned int i, d;

    i = 0;
    do
    {
        d = n % 16;
        if (d < 10)
        {
            s[i++] = d + '0';
        }
        else
        {
            s[i++] = d - 10 + 'a';
        }
    } while ((n /= 16) > 0);
    s[i] = 0;
    reverse(s);

    return i;
}

char *strtok(char *str, const char *delimiters)
{
    static char *sp = NULL;
    int i = 0;
    int len = strlen(delimiters);
    if (!str && !sp)
    {
        return 0;
    }

    if (str != NULL && sp == NULL)
    {
        sp = str;
    }

    char *p_start = sp;
    while (1)
    {
        for (i = 0; i < len; i++)
        {
            if (*p_start == delimiters[i])
            {
                p_start++;
                break;
            }
        }

        if (i == len)
        {
            sp = p_start;
            break;
        }
    }

    if (*sp == '\0')
    {
        sp = 0;
        return sp;
    }

    while (*sp != '\0')
    {
        for (i = 0; i < len; i++)
        {
            if (*sp == delimiters[i])
            {
                *sp = '\0';
                break;
            }
        }

        sp++;
        if (i < len)
        {
            break;
        }
    }

    return p_start;
}