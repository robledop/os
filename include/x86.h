#pragma once
#include <types.h>

// Interrupt flags enabled
#define EFLAGS_IF 0x00000200

static inline uint32_t read_eflags(void)
{
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

static inline void load_gs(uint16_t v)
{
    asm volatile("movw %0, %%gs" : : "r"(v));
}

static inline void cli(void)
{
    asm volatile("cli");
}

static inline void sti(void)
{
    asm volatile("sti");
}

static inline void hlt(void)
{
    asm volatile("hlt");
}

static inline void pause(void)
{
    asm volatile("pause");
}

static inline uint32_t xchg(volatile uint32_t *addr, uint32_t newval)
{
    uint32_t result;

    asm volatile("lock; xchgl %0, %1" : "+m"(*addr), "=a"(result) : "1"(newval) : "cc");
    return result;
}

static inline uint32_t read_cr2(void)
{
    uint32_t val;
    asm volatile("movl %%cr2,%0" : "=r"(val));
    return val;
}

static inline void lcr3(uint32_t val)
{
    asm volatile("movl %0,%%cr3" : : "r"(val));
}
