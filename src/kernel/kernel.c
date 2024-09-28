#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "terminal/terminal.h"
#include "memory/heap/kheap.h"

// Divide by zero error
extern void cause_problem();

void kernel_main()
{
    terminal_clear();
    print("Hello, World! \nThis is a new line!");

    kheap_init();

    idt_init();

    void *ptr = kmalloc(100);
    void *ptr2 = kmalloc(500);
    void *ptr3 = kmalloc(5000);

    kfree(ptr);

    void *ptr4 = kmalloc(1000);

    int a = 1;
    int b = 2;
    int c = a + b;

    if (ptr == 0 || ptr2 == 0 || ptr3 == 0 || c != 3 || ptr4 == 0)   
    {
        print("Failed to allocate memory\n");
    }
}