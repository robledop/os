#pragma once

#ifdef __KERNEL__
#error "This is a user-space header file. It should not be included in the kernel."
#endif

#include <stddef.h>
#include <stdint.h>

enum PROCESS_STATE { RUNNING, ZOMBIE, WAITING, TERMINATED };

void *malloc(size_t size);
void *calloc(int number_of_items, int size);
__attribute__((nonnull))
void free(void *ptr);
int waitpid(int pid, const int *return_status);
int wait(const int *return_status);
void reboot(void);
void shutdown(void);
int fork(void);
int exec(const char path[static 1], const char **args);
int getpid(void);
int create_process(const char path[static 1], const char *current_directory);
void sleep(uint32_t milliseconds);
void yield(void);
void ps(void);
