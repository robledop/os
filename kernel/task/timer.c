#include <idt.h>
#include <io.h>
#include <pic.h>
#include <pit.h>
#include <stddef.h>
#include <timer.h>

void timer_callback(struct interrupt_frame *frame);
volatile uint32_t timer_tick;

typedef void (*voidfunc_t)();

#define MAX_CALLBACKS 8
static size_t callback_count = 0;
static voidfunc_t callbacks[MAX_CALLBACKS];

/**
 * Sleep Timer Non-Busy Waiting Idea:
 * Create a struct that contains the end time and the callback
 * function pointer that should be called when timer_tick = end
 * After each timer_tick we check our end time and call the function
 * if we're equal.
 */

void timer_init(const uint32_t freq)
{
    pit_set_interval(freq);

    idt_register_interrupt_callback(0x20, timer_callback);
}

void timer_callback(struct interrupt_frame *frame)
{
    (void)frame;
    timer_tick = timer_tick + 1;
    for (size_t i = 0; i < callback_count; i++) {
        callbacks[i]();
    }
}

void sleep(const uint32_t ms)
{
    const uint32_t start = timer_tick;
    const uint32_t final = start + ms;
    while (timer_tick < final)
        ;
}

void timer_register_callback(void (*func)())
{
    if (callback_count < MAX_CALLBACKS - 1) {
        callbacks[callback_count] = func;
        callback_count++;
    }
}
