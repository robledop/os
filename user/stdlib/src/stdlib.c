#include "stdlib.h"
#include <os.h>
#include <string.h>
#include "../../../src/include/syscall.h"
#include "syscall.h"

void *malloc(size_t size)
{
    return (void *)syscall1(SYSCALL_MALLOC, size);
}

/// @brief Allocate memory for an array of elements and set the memory to zero
/// @param number_of_items number of elements
/// @param size size of each element
void *calloc(const int number_of_items, const int size)
{
    return (void *)syscall2(SYSCALL_CALLOC, number_of_items, size);
}

void free(void *ptr)
{
    syscall1(SYSCALL_FREE, ptr);
}

int waitpid(const int pid, const int *return_status)
{
    return syscall2(SYSCALL_WAIT_PID, pid, return_status);
}

int wait(const int *return_status)
{
    return syscall2(SYSCALL_WAIT_PID, -1, return_status);
}

void reboot()
{
    syscall0(SYSCALL_REBOOT);
}

void shutdown()
{
    syscall0(SYSCALL_SHUTDOWN);
}

int fork()
{
    return syscall0(SYSCALL_FORK);
}

int exec(const char *path, const char **args)
{
    return syscall2(SYSCALL_EXEC, path, args);
}

int getpid()
{
    return syscall0(SYSCALL_GET_PID);
}

int create_process(const char *path, const char *current_directory)
{
    char buffer[1024];
    strncpy(buffer, path, sizeof(buffer));
    struct command_argument *root_command_argument = os_parse_command(buffer, sizeof(buffer));
    if (root_command_argument == NULL) {
        return -1;
    }

    if (root_command_argument->current_directory == NULL) {
        root_command_argument->current_directory = calloc(1, MAX_PATH_LENGTH);
    }

    strncpy(root_command_argument->current_directory, current_directory, MAX_PATH_LENGTH);

    return syscall1(SYSCALL_CREATE_PROCESS, root_command_argument);
}
