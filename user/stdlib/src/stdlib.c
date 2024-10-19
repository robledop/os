#include "stdlib.h"
#include "../../../src/include/syscall.h"
#include "os.h"
#include "syscall.h"

void *malloc(size_t size) { return (void *)syscall1(SYSCALL_MALLOC, size); }
void free(void *ptr) { syscall1(SYSCALL_FREE, ptr); }
int waitpid(const int pid) { return syscall1(SYSCALL_WAIT_PID, pid); }
void reboot() { syscall0(SYSCALL_REBOOT); }
void shutdown() { syscall0(SYSCALL_SHUTDOWN); }
