#pragma once

#include "../../../src/include/syscall.h"

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
