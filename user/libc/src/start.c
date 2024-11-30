#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

#define BUFSIZ 1024

extern int main(int argc, char **argv);

FILE *stdin;
FILE *stdout;
FILE *stderr;

void init_standard_streams();
void c_start()
{
    struct process_arguments arguments;
    syscall1(SYSCALL_GET_PROGRAM_ARGUMENTS, &arguments);

    open("/dev/tty", O_RDONLY); // stdin
    open("/dev/tty", O_WRONLY); // stdout
    open("/dev/tty", O_WRONLY); // stderr

    init_standard_streams();

    main(arguments.argc, arguments.argv);
}

void init_standard_streams()
{
    stdin  = malloc(sizeof(FILE));
    stdout = malloc(sizeof(FILE));
    stderr = malloc(sizeof(FILE));

    // Initialize stdin
    stdin->fd              = 0; // File descriptor for standard input
    stdin->buffer_size     = BUFSIZ;
    stdin->buffer          = malloc(stdin->buffer_size);
    stdin->pos             = 0;
    stdin->bytes_available = 0;
    stdin->eof             = 0;
    stdin->error           = 0;
    stdin->mode            = O_RDONLY;

    // Initialize stdout
    stdout->fd              = 1; // File descriptor for standard output
    stdout->buffer_size     = BUFSIZ;
    stdout->buffer          = malloc(stdout->buffer_size);
    stdout->pos             = 0;
    stdout->bytes_available = 0;
    stdout->eof             = 0;
    stdout->error           = 0;
    stdout->mode            = O_WRONLY;

    // Initialize stderr (unbuffered)
    stderr->fd              = 2; // File descriptor for standard error
    stderr->buffer_size     = 0;
    stderr->buffer          = nullptr; // Unbuffered
    stderr->pos             = 0;
    stderr->bytes_available = 0;
    stderr->eof             = 0;
    stderr->error           = 0;
    stderr->mode            = O_WRONLY;
}