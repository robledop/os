#pragma once
#include <stdint.h>

#define TIMER_COMMAND_PORT 0x43
#define TIMER_DATA_PORT 0x40

extern volatile uint32_t timer_tick;

/**
 * @brief Initialize the CPU timer with the given frequency.
 *
 * @param freq Timer frequency
 */
void timer_init(uint32_t freq);
/**
 * @brief Sleeps for a certain length of time.
 *
 * @param ms Sleep length in milliseconds
 */
void sleep(uint32_t ms);

void timer_register_callback(void (*func)());
