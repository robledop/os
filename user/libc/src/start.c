#include "stdio.h"
#include "syscall.h"

extern int main(int argc, char **argv);

void c_start()
{
    struct process_arguments arguments;
    syscall1(SYSCALL_GET_PROGRAM_ARGUMENTS, &arguments);
    open("/dev/tty", O_WRONLY);

    main(arguments.argc, arguments.argv);
}