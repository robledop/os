#include "pit.h"
#include "io.h"

// https://wiki.osdev.org/Programmable_Interval_Timer

#define PIT_CHANNEL_0_DATA 0x40
#define PIT_CHANNEL_1_DATA 0x41
#define PIT_CHANNEL_2_DATA 0x42
#define PIT_COMMAND 0x43

#define PIT_FREQUENCY 1193182 // Hz

void pit_init()
{
    /* name | value | size | desc
     * --------------------------
     * chan |     0 |    2 | the channel to use, channel 0 = IRQ0
     * acs  |   0x3 |    2 | how the divider is sent, 3 = lobyte then hibyte
     * mode |   0x3 |    3 | the mode of the pit, mode 3 = square wave
     * bcd  |     0 |    1 | bcd or binary mode, 0 = binary, 1 = bcd
     */
    constexpr uint8_t data = (1 << 5) | (1 << 4) | (1 << 2) | (1 << 1) | 0x00;
    outb(PIT_COMMAND, data);
}

/// @brief Set the interval of the PIT
/// @param interval The interval in milliseconds
void pit_set_interval(const uint32_t interval)
{
    const uint32_t frequency = 1000 / interval;
    const uint16_t divider   = (uint16_t)(PIT_FREQUENCY / frequency);

    outb(PIT_CHANNEL_0_DATA, (uint8_t)divider);
    outb(PIT_CHANNEL_0_DATA, (uint8_t)(divider >> 8));
}
