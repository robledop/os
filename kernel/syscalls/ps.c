#include <printf.h>
#include <scheduler.h>
#include <string.h>
#include <syscall.h>
#include <termcolors.h>

// TODO: Re-implement this in userland using a device file
void *sys_ps(struct interrupt_frame *frame)
{
    struct process_info *proc_info = nullptr;
    int count                      = 0;
    scheduler_get_processes(&proc_info, &count);
    printf(KBBLU "\n %-5s%-15s%-12s%-12s%-12s\n" KWHT, "PID", "Name", "Priority", "State", "Exit code");
    for (int i = 0; i < count; i++) {
        constexpr int col               = 15;
        const struct process_info *info = &proc_info[i];

        char state[col + 1];
        switch (info->state) {
        case RUNNING:
            strncpy(state, "RUNNING", col);
            break;
        case ZOMBIE:
            strncpy(state, "ZOMBIE", col);
            break;
        case SLEEPING:
            strncpy(state, "SLEEPING", col);
            break;
        case WAITING:
            strncpy(state, "WAITING", col);
            break;
        default:
            strncpy(state, "UNKNOWN", col);
            break;
        }

        printf(" %-5u%-15s%-12u%-12s%-12u\n", info->pid, info->file_name, info->priority, state, info->exit_code);
    }

    return nullptr;
}
