#pragma once

#ifdef __KERNEL__
#error "This is a user-space header file. It should not be included in the kernel."
#endif

#include "types.h"

enum PROCESS_STATE { RUNNING, ZOMBIE, WAITING, TERMINATED };

void *malloc(size_t size);
void *calloc(int number_of_items, int size);
void free(void *ptr);
int waitpid(int pid, const int *return_status);
int wait(const int *return_status);
void reboot();
void shutdown();
int fork();
int exec(const char *path, const char **args);
int getpid();
int create_process(const char *path, const char *current_directory);
void sleep(const uint32_t milliseconds);
void yield();
void ps();
