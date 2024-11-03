#include <spinlock.h>
#include <x86.h>

void spinlock_init(spinlock_t *lock)
{
    *lock = 0;
}

void spin_lock(spinlock_t *lock)
{
    while (1) {
        if (!__atomic_load_n(lock, __ATOMIC_RELAXED)) {
            if (!__atomic_test_and_set(lock, __ATOMIC_ACQUIRE)) {
                return;
            }
        }
        pause();
    }
}

void spin_unlock(spinlock_t *lock)
{
    __sync_synchronize();

    __atomic_clear(lock, __ATOMIC_RELEASE);
}