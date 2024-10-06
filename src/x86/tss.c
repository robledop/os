#include "tss.h"
#include "gdt.h"
#include "memory.h"
#include "config.h"

// struct tss tss;

// void tss_load(int tss_selector)
// {
//     asm volatile("ltr %0" : : "r"(tss_selector));
// }

// void tss_init()
// {
//     memset(&tss, 0, sizeof(tss));
//     tss.esp0 = 0x60000;
//     tss.ss0 = 0x10;
//     tss.iopb = sizeof(struct tss);

//     tss_load(0x28);
// }

#define LOAD_TSS() asm volatile ("ltr %0" : : "m" (tss_selector))

struct tss_entry tss;

void init_tss(void)
{
    uint16_t tss_selector = GDT_KERNEL_TSS;
    tss.ldt_selector = 0;
    tss.prev_tss = GDT_KERNEL_TSS;
    tss.iomap_base = sizeof(struct tss_entry);
    tss.esp_0 = 0x60000;
    tss.ss_0 = 0x10;

    LOAD_TSS();
}
