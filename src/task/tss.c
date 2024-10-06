#include "tss.h"
#include "gdt.h"
// tss_load:
//     push ebp
//     mov ebp, esp
//     mov ax, [ebp + 8] ; Load the TSS address
//     ltr ax
//     pop ebp
//     ret

// void tss_load(int selector)
// {
//     asm volatile (
//         "mov %0, %%ax\n"
//         "ltr %%ax\n"
//         :
//         : "r" (selector)
//         : "ax"
//     );

// }

// void tss_load(int tss_selector) {
//     asm volatile("ltr %0" : : "r" (tss_selector));
// }

#define FLUSH_TSS() asm volatile ("ltr %0" : : "m" (tss_selector))

struct tss_entry tss;

void init_tss(void)
{
    uint16_t tss_selector = GDT_KERNEL_TSS;
    tss.ldt_selector = 0;
    tss.prev_tss = GDT_KERNEL_TSS;
    tss.iomap_base = sizeof(struct tss_entry);

    FLUSH_TSS();
}