#pragma once
#include "types.h"

void *malloc(size_t size);
void free(void *ptr);
int waitpid(int pid);
void reboot();
void shutdown();
