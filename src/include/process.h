#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include "config.h"
#include "task.h"
#include "types.h"

#define PROCESS_FILE_TYPE_ELF 0
#define PROCESS_FILE_TYPE_BINARY 1
typedef unsigned char PROCESS_FILE_TYPE;

struct command_argument {
    char argument[512];
    struct command_argument *next;
    char *current_directory;
};

struct process_allocation {
    void *ptr;
    size_t size;
};

struct process_arguments {
    int argc;
    char **argv;
};

struct process {
    uint16_t pid;
    char file_name[MAX_PATH_LENGTH];
    struct task *task;
    struct process_allocation allocations[MAX_PROGRAM_ALLOCATIONS];
    PROCESS_FILE_TYPE file_type;
    union {
        void *pointer;
        struct elf_file *elf_file;
    };

    void *stack;
    uint32_t size;

    struct keyboard_buffer {
        uchar buffer[KEYBOARD_BUFFER_SIZE];
        int tail;
        int head;

    } keyboard;

    struct process_arguments arguments;
    char *current_directory;
    uint8_t console;
};

int process_load_switch(const char *file_name, struct process **process);
int process_switch(struct process *process);
int process_load(const char *file_name, struct process **process);
int process_load_for_slot(const char *file_name, struct process **process, uint16_t slot);
struct process *process_current();
struct process *process_get(int pid);
void *process_malloc(struct process *process, size_t size);
void process_free(struct process *process, void *ptr);
void process_get_arguments(struct process *process, int *argc, char ***argv);
int process_inject_arguments(struct process *process, struct command_argument *root_argument);
int process_terminate(struct process *process);
int process_set_current_directory(struct process *process, const char *directory);

#endif