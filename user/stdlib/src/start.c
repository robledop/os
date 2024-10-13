#include "os.h"
#include "stdio.h"

extern int main(int argc, char **argv);

void c_start()
{
    struct process_arguments arguments;
    os_process_get_arguments(&arguments);

    main(arguments.argc, arguments.argv);
}