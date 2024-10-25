#pragma once

#include "types.h"

typedef uint32_t spinlock_t;

#define lock(lk, code)                                                                                                 \
    {                                                                                                                  \
        acquire_lock(&lk);                                                                                             \
        code;                                                                                                          \
        release_lock(&lk);                                                                                             \
    }

void spinlock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
