)#pragma once

#include "types.h"

typedef uint32_t spinlock_t;

#define lock(lk, code)                                                                                                  \
    {                                                                                                                  \
        acquire_lock(&lk);                                                                                              \
        code;                                                                                                          \
        release_lock(&lk);                                                                                              \
    }

// defined in task/task.asm
void release_lock(spinlock_t *lock);
void acquire_lock(spinlock_t *lock);