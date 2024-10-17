#include "console.h"

#include <kernel.h>

#include "memory.h"
#include "terminal.h"

Console consoles[NUM_CONSOLES];
int active_console = 0;

void initialize_consoles() {
    for (int i = 0; i < NUM_CONSOLES; i++) {
        memset(consoles[i].framebuffer, 0, FRAMEBUFFER_SIZE);
        consoles[i].input_buffer_head = 0;
        consoles[i].input_buffer_tail = 0;
        start_shell(i); // Pass console index to the shell
    }
}

// void handle_input(Console *console, char input_char) {
//     // Process the input character
//     // Update the console's framebuffer buffer accordingly
//     render_character_to_buffer(console->framebuffer, input_char, &cursor_position);
//     if (console == &consoles[active_console]) {
//         // Update the physical framebuffer if this console is active
//         update_display_region(physical_framebuffer, console->framebuffer, updated_region);
//     }
// }


void switch_console(int console_number) {
    if (console_number >= 0 && console_number < NUM_CONSOLES) {
        active_console = console_number;
        // Update the physical framebuffer with the new console's buffer
        memcpy((void *)VIDEO_MEMORY, consoles[active_console].framebuffer, FRAMEBUFFER_SIZE);
    }
}