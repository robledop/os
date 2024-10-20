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
    } else {
        // waitpid(rc);
        printf("After forking. Parent of %d (pid:%d)\n", rc, getpid());
    }

    return 0;
}
