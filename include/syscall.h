#pragma once

enum SysCalls {
    SYSCALL_EXIT,
    SYSCALL_GETKEY,
    SYSCALL_PUTCHAR,
    SYSCALL_PRINT,
    SYSCALL_MALLOC,
    SYSCALL_CALLOC,
    SYSCALL_FREE,
    SYSCALL_CREATE_PROCESS,
    SYSCALL_FORK,
    SYSCALL_EXEC,
    SYSCALL_GET_PID,
    SYSCALL_GET_PROGRAM_ARGUMENTS,
    SYSCALL_OPEN,
    SYSCALL_CLOSE,
    SYSCALL_STAT,
    SYSCALL_READ,
    SYSCALL_OPEN_DIR,
    SYSCALL_SET_CURRENT_DIRECTORY,
    SYSCALL_GET_CURRENT_DIRECTORY,
    SYSCALL_WAIT_PID,
    SYSCALL_REBOOT,
    SYSCALL_SHUTDOWN,
    SYSCALL_SLEEP,
    SYSCALL_YIELD,
    SYSCALL_PS,      // TODO: I should use a device file for this instead
    SYSCALL_MEMSTAT, // TODO: I should use a device file for this instead
};

#ifdef __KERNEL__
void register_syscalls();

struct interrupt_frame;

[[noreturn]] void *sys_exit(struct interrupt_frame *frame);
void *sys_getkey(struct interrupt_frame *frame);
void *sys_putchar(struct interrupt_frame *frame);
void *sys_print(struct interrupt_frame *frame);
void *sys_malloc(struct interrupt_frame *frame);
void *sys_free(struct interrupt_frame *frame);
void *sys_create_process(struct interrupt_frame *frame);
void *sys_get_program_arguments(struct interrupt_frame *frame);
void *sys_open(struct interrupt_frame *frame);
void *sys_close(struct interrupt_frame *frame);
void *sys_stat(struct interrupt_frame *frame);
void *sys_read(struct interrupt_frame *frame);
void *sys_open_dir(struct interrupt_frame *frame);
void *sys_set_current_directory(struct interrupt_frame *frame);
void *sys_get_current_directory(struct interrupt_frame *frame);
void *sys_wait_pid(struct interrupt_frame *frame);
void *sys_reboot(struct interrupt_frame *frame);
void *sys_shutdown(struct interrupt_frame *frame);
void *sys_calloc(struct interrupt_frame *frame);
void *sys_fork(struct interrupt_frame *frame);
void *sys_exec(struct interrupt_frame *frame);
void *sys_getpid(struct interrupt_frame *frame);
void *sys_sleep(struct interrupt_frame *frame);
void *sys_yield(struct interrupt_frame *frame);
void *sys_ps(struct interrupt_frame *frame);
void *sys_memstat(struct interrupt_frame *frame);

#else

/// @brief Invokes syscall NUMBER, passing no arguments, and returns the return value as an 'int'.
#define syscall0(NUMBER)                                                                                               \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("movl %[number], %%eax;"                                                                          \
                     "int $0x80;"                                                                                      \
                     : "=a"(retval)                                                                                    \
                     : [number] "i"(NUMBER)                                                                            \
                     : "memory");                                                                                      \
        retval;                                                                                                        \
    })

/// @brief Invokes syscall NUMBER, passing argument ARG0, and returns the return value as an 'int'.
#define syscall1(NUMBER, ARG0)                                                                                         \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("pushl %[arg0];"                                                                                  \
                     "movl %[number], %%eax;"                                                                          \
                     "int $0x80;"                                                                                      \
                     "addl $4, %%esp"                                                                                  \
                     : "=a"(retval)                                                                                    \
                     : [number] "i"(NUMBER), [arg0] "g"(ARG0)                                                          \
                     : "memory");                                                                                      \
        retval;                                                                                                        \
    })

/// @brief Invokes syscall NUMBER, passing arguments ARG0 and ARG1, and returns the return value as an 'int'.
#define syscall2(NUMBER, ARG0, ARG1)                                                                                   \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("pushl %[arg0];"                                                                                  \
                     "pushl %[arg1]; "                                                                                 \
                     "movl %[number], %%eax;"                                                                          \
                     "int $0x80;"                                                                                      \
                     "addl $8, %%esp"                                                                                  \
                     : "=a"(retval)                                                                                    \
                     : [number] "i"(NUMBER), [arg0] "r"(ARG0), [arg1] "r"(ARG1)                                        \
                     : "memory");                                                                                      \
        retval;                                                                                                        \
    })

/// @brief Invokes syscall NUMBER, passing arguments ARG0, ARG1, and ARG2, and returns the return value as an 'int'.
#define syscall3(NUMBER, ARG0, ARG1, ARG2)                                                                             \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("pushl %[arg0];"                                                                                  \
                     "pushl %[arg1];"                                                                                  \
                     "pushl %[arg2];"                                                                                  \
                     "movl %[number], %%eax;"                                                                          \
                     "int $0x80;"                                                                                      \
                     "addl $12, %%esp"                                                                                 \
                     : "=a"(retval)                                                                                    \
                     : [number] "i"(NUMBER), [arg0] "r"(ARG0), [arg1] "r"(ARG1), [arg2] "r"(ARG2)                      \
                     : "memory");                                                                                      \
        retval;                                                                                                        \
    })

/// @brief Invokes syscall NUMBER, passing arguments ARG0, ARG1, ARG2, and ARG3, and returns the return value as an
/// 'int'.
#define syscall4(NUMBER, ARG0, ARG1, ARG2, ARG3)                                                                       \
    ({                                                                                                                 \
        int retval;                                                                                                    \
        asm volatile("pushl %[arg0];"                                                                                  \
                     "pushl %[arg1];"                                                                                  \
                     "pushl %[arg2];"                                                                                  \
                     "pushl %[arg3];"                                                                                  \
                     "movl %[number], %%eax;"                                                                          \
                     "int $0x80;"                                                                                      \
                     "addl $16, %%esp"                                                                                 \
                     : "=a"(retval)                                                                                    \
                     : [number] "i"(NUMBER), [arg0] "r"(ARG0), [arg1] "r"(ARG1), [arg2] "r"(ARG2), [arg3] "r"(ARG3)    \
                     : "memory");                                                                                      \
        retval;                                                                                                        \
    })
#endif