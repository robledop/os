#ifndef ASSERT_H    
#define ASSERT_H

#include "terminal.h"

#define ASSERT(condition) if (!(condition)) { \
    kprintf(KRED "Assertion failed in %s on line %d\n" KWHT, __FILE__, __LINE__); \
    while(1); \
}

#endif