#include <sleeplock.h>

void sleeplock_init(struct sleeplock *sleeplock, char *name)
{
    sleeplock->lock = 0;
    sleeplock->name = name;
}

void sleeplock_acquire(struct sleeplock *lock)
{
}

void sleeplock_release(struct sleeplock *lock)
{
}