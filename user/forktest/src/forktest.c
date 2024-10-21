#include "stdio.h"
#include "stdlib.h"

int main(const int argc, char **argv)
{
    printf("\nBefore forking, (pid:%d)\n", getpid());

    const int rc = fork();

    if (rc < 0) {
        printf("Fork failed\n");
        exit();
    } else if (rc == 0) {
        printf("Child (pid:%d)\n", getpid());
        printf("Child will exec blank.elf\n");
        exec("0:/bin/blank.elf", nullptr);

        printf("This should not be printed\n");
    } else {
        waitpid(rc, ZOMBIE);
        printf("\nAfter forking. Parent of %d (pid:%d)\n", rc, getpid());
    }

    return 0;
}
