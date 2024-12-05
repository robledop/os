#pragma once

#include <stddef.h>

enum SysCalls {
    SYSCALL_EXIT,
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
    SYSCALL_WRITE,
    SYSCALL_LSEEK,
    SYSCALL_MKDIR,
    SYSCALL_GETDENTS, // Get directory entries
    SYSCALL_CHDIR,    // Change directory
    SYSCALL_GETCWD,
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

[[noreturn]] void *sys_exit(void);
void *sys_getkey(void);
void *sys_putchar(void);
void *sys_print(void);
void *sys_malloc(void);
void *sys_free(void);
void *sys_create_process(void);
void *sys_get_program_arguments(void);
void *sys_open(void);
void *sys_close(void);
void *sys_stat(void);
void *sys_read(void);
void *sys_write(void);
void *sys_lseek(void);
void *sys_mkdir(void);
void *sys_getdents(void);
void *sys_chdir(void);
void *sys_getcwd(void);
void *sys_wait_pid(void);
void *sys_reboot(void);
void *sys_shutdown(void);
void *sys_calloc(void);
void *sys_fork(void);
void *sys_exec(void);
void *sys_getpid(void);
void *sys_sleep(void);
void *sys_yield(void);
void *sys_ps(void);
void *sys_memstat(void);

void *get_pointer_argument(int index);
int get_integer_argument(int index);
char *get_string_argument(int index, size_t max_len);
struct command_argument *parse_command(char **args);

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