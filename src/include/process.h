#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "config.h"
#include "task.h"

#define PROCESS_FILE_TYPE_ELF 0
#define PROCESS_FILE_TYPE_BINARY 1
typedef unsigned char PROCESS_FILE_TYPE;
struct process
{
    uint16_t pid;
    char file_name[MAX_PATH_LENGTH];
    struct task *task;
    void *allocations[MAX_PROGRAM_ALLOCATIONS];
    PROCESS_FILE_TYPE file_type;
    union
    {
        void *pointer;
        struct elf_file *elf_file;
    };

    void *stack;
    uint32_t size;

    struct keyboard_buffer
    {
        char buffer[KEYBOARD_BUFFER_SIZE];
        int tail;
        int head;

    } keyboard;
};

int process_load_switch(const char *file_name, struct process **process);
int process_switch(struct process *process);
int process_load(const char *file_name, struct process **process);
int process_load_for_slot(const char *file_name, struct process **process, uint16_t slot);
struct process *process_current();
struct process *process_get(int pid);

#endif