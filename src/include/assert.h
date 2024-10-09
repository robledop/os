#ifndef ASSERT_H
#define ASSERT_H

#include "terminal.h"
#include "kernel.h"

#define DEBUG_ASSERT

#ifdef DEBUG_ASSERT
#define ASSERT(condition, message)                                                    \
    if (!(condition))                                                                 \
    {                                                                                 \
        kprintf(KRED "Assertion failed in %s on line %d\n" KWHT, __FILE__, __LINE__); \
        panic(message);                                                               \
    }
#else
#define ASSERT(condition, message)
#endif

#endif