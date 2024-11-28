#pragma once

#include <config.h>
#include <thread.h>

#define PROCESS_FILE_TYPE_ELF 0
#define PROCESS_FILE_TYPE_BINARY 1
typedef unsigned char PROCESS_FILE_TYPE;
enum PROCESS_STATE {
    EMPTY,
    /// The process is ready to run
    RUNNING,
    /// The process has been deallocated, but the parent process has not yet waited for it
    ZOMBIE,
    /// The process is waiting for a child process to exit
    WAITING,
    /// The process is waiting for a signal
    SLEEPING
};
enum PROCESS_SIGNAL { NONE, SIGKILL, SIGSTOP, SIGCONT, SIGTERM, SIGWAKEUP };
enum SLEEP_REASON { SLEEP_REASON_NONE, SLEEP_REASON_KEYBOARD };

struct command_argument {
    char argument[512];
    struct command_argument *next;
    char current_directory[MAX_PATH_LENGTH];
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
    int rand_id; // For debugging purposes
    int tty_fd;
    int priority;
    char file_name[MAX_PATH_LENGTH];
    struct page_directory *page_directory;
    struct process *parent;
    // TODO: Abstract this into a separate data structure for linked lists
    struct process *next;
    struct list_elem elem;
    struct process *children;
    struct thread *thread;
    enum PROCESS_STATE state;
    enum PROCESS_SIGNAL signal;
    enum SLEEP_REASON sleep_reason;
    int wait_pid;
    int exit_code;
    uint32_t sleep_until;
    struct process_allocation allocations[MAX_PROGRAM_ALLOCATIONS];
    PROCESS_FILE_TYPE file_type;
    union {
        void *pointer;
        struct elf_file *elf_file;
    };

    struct file *file_descriptors[MAX_FILE_DESCRIPTORS];
    void *stack;
    uint32_t size;

    struct keyboard_buffer {
        uint8_t buffer[KEYBOARD_BUFFER_SIZE];
        int tail;
        int head;

    } keyboard;

    struct process_arguments arguments;
    char *current_directory;

    // TODO: Add file descriptors
};
__attribute__((nonnull)) int process_load_enqueue(const char file_name[static 1], struct process **process);
__attribute__((nonnull)) int process_load(const char file_name[static 1], struct process **process);
__attribute__((nonnull)) int process_load_for_slot(const char file_name[static 1], struct process **process,
                                                   uint16_t pid);
__attribute__((nonnull)) void *process_malloc(struct process *process, size_t size);
__attribute__((nonnull)) void *process_calloc(struct process *process, size_t nmemb, size_t size);
__attribute__((nonnull)) void process_free(struct process *process, void *ptr);
__attribute__((nonnull)) void process_get_arguments(struct process *process, int *argc, char ***argv);
__attribute__((nonnull)) int process_inject_arguments(struct process *process,
                                                      const struct command_argument *root_argument);
__attribute__((nonnull)) int process_zombify(struct process *process);
__attribute__((nonnull)) int process_set_current_directory(struct process *process, const char directory[static 1]);
__attribute__((nonnull)) int process_wait_pid(struct process *process, int pid);
__attribute__((nonnull)) struct process *find_child_process_by_pid(const struct process *parent, int pid);
__attribute__((nonnull)) struct process *find_child_process_by_state(const struct process *parent,
                                                                     enum PROCESS_STATE state);
__attribute__((nonnull)) int process_add_child(struct process *parent, struct process *child);
__attribute__((nonnull)) int process_remove_child(struct process *parent, const struct process *child);
__attribute__((nonnull)) struct process *process_clone(struct process *process);
__attribute__((nonnull)) int process_load_data(const char file_name[static 1], struct process *process);
__attribute__((nonnull)) int process_map_memory(struct process *process);
__attribute__((nonnull)) int process_unmap_memory(const struct process *process);
__attribute__((nonnull)) int process_free_allocations(struct process *process);
__attribute__((nonnull)) int process_free_program_data(const struct process *process);
__attribute__((nonnull)) void process_command_argument_free(struct command_argument *argument);

struct file *process_get_file_descriptor(const struct process *process, uint32_t index);
int process_new_file_descriptor(struct process *process, struct file **desc_out);
void process_free_file_descriptor(struct process *process, struct file *desc);
