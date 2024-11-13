#pragma once

#include <stdint.h>

// https://wiki.osdev.org/Programmable_Interval_Timer

void pit_init(void);
void pit_set_interval(uint32_t interval);
