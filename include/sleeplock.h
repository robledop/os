#pragma once

#include <spinlock.h>
#include <task.h>

struct sleeplock {
    spinlock_t lock;
    struct task_list waiting;
    char *name;
};

void sleeplock_init(struct sleeplock *sleeplock, char *name);
void sleeplock_acquire(struct sleeplock *lock);
void sleeplock_release(struct sleeplock *lock);
