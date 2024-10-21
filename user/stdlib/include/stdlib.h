#pragma once
#include "types.h"

enum PROCESS_STATE { RUNNING, ZOMBIE, WAITING, TERMINATED };

void *malloc(size_t size);
void *calloc(int nmemb, int size);
void free(void *ptr);
int waitpid(int pid, enum PROCESS_STATE state);
int wait(enum PROCESS_STATE state);
void reboot();
void shutdown();
int fork();
int exec(const char* path, const char* argv[]);
int getpid();
int create_process(const char *command, const char *current_directory);
