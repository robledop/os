#pragma once
#include <stdint.h>

// Interrupt flags enabled
#define EFLAGS_CF 0x00000001
#define EFLAGS_PF 0x00000004
#define EFLAGS_AF 0x00000010
#define EFLAGS_ZF 0x00000040
#define EFLAGS_SF 0x00000080
#define EFLAGS_TF 0x00000100
#define EFLAGS_IF 0x00000200
#define EFLAGS_DF 0x00000400
#define EFLAGS_OF 0x00000800
#define EFLAGS_IOPL 0x00003000
#define EFLAGS_NT 0x00004000
#define EFLAGS_RF 0x0001'0000
#define EFLAGS_VM 0x0002'0000
#define EFLAGS_AC 0x0004'0000
#define EFLAGS_VIF 0x0008'0000
#define EFLAGS_VIP 0x0010'0000
#define EFLAGS_ID 0x0020'0000
#define EFLAGS_AI 0x8000'0000
#define EFLAGS_ALL                                                                                                     \
    (EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | EFLAGS_TF | EFLAGS_IF | EFLAGS_DF | EFLAGS_OF |       \
     EFLAGS_IOPL | EFLAGS_NT | EFLAGS_RF | EFLAGS_VM | EFLAGS_AC | EFLAGS_VIF | EFLAGS_VIP | EFLAGS_ID | EFLAGS_AI)

static inline uint32_t read_eflags(void)
{
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

static inline uint32_t read_esp(void)
{
    uint32_t esp;
    asm volatile("movl %%esp, %0" : "=r"(esp));
    return esp;
}

static inline void load_gs(uint16_t v)
{
    asm volatile("movw %0, %%gs" : : "r"(v));
}

/// @brief Disable interrupts
static inline void cli(void)
{
    asm volatile("cli");
}

/// @brief Enable interrupts
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

static inline uint64_t rdtsc(void)
{
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}
