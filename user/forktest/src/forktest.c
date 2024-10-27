#include "stdio.h"
#include "stdlib.h"

int main(const int argc, char **argv)
{
    printf(KYEL "\nBefore forking, (pid:%d)\n", getpid());

    const int rc = fork();


    if (rc < 0) {
        printf("Fork failed\n");
    } else if (rc == 0) {
        printf(KCYN "Child (pid:%d)\n", getpid());
        printf("Child will exec blank.elf" KWHT " ");
        exec("0:/bin/blank.elf", nullptr);

        printf("This should not be printed\n");
    } else {
        wait(nullptr);
        printf(KYEL "\nAfter forking. Parent of %d (pid:%d)\n", rc, getpid());
    }
    exit();

    return 0;
}
