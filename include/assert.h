#pragma once

#define static_assert _Static_assert

void abort(void);

void assert(const char *snippet, const char *file, int line, const char *message, ...);

#define ASSERT(cond, ...)                                                                                              \
    if (!(cond))                                                                                                       \
    assert(#cond, __FILE__, __LINE__, #__VA_ARGS__ __VA_OPT__(, )##__VA_ARGS__)
