#pragma once

#include <spinlock.h>
#include <stdint.h>

struct sleeplock {
    uint32_t locked;
    spinlock_t lk;

    char *name;
    int pid;
};
