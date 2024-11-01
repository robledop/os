#include "stdio.h"
#include "stdlib.h"

int main(const int argc, char **argv)
{
    printf(KRESET KYEL "\nBefore forking, (pid:%d)\n", getpid());

    const int rc = fork();

    if (rc < 0) {
        printf("Fork failed\n");
    } else if (rc == 0) {
        printf(KCYN "Child (pid:%d)\n", getpid());
        printf("Child will exec blank.elf" KWHT " ");
        exec("0:/bin/blank.elf", nullptr);

        printf("This should not be printed\n");
    } else {
        waitpid(rc, nullptr);
        printf(KYEL "\nAfter forking. Parent of %d (pid:%d)", rc, getpid());
    }

    for (int i = 0; i < 10; i++) {
        const char *current_directory = get_current_directory();
        printf("create_process: %d\n", i);
        const int pid = create_process((char *)"sleep 100", current_directory);
        if (pid < 0) {
        } else {
            waitpid(pid, nullptr);
        }
    }

    for (int i = 0; i < 10; i++) {
        const int r = fork();
        if (r < 0) {
            print("Fork failed\n");
        } else if (r == 0) {
            printf("Forked child %d (pid:%d)\n", i, getpid());
        } else {
            waitpid(r, nullptr);
            printf("Parent of %d (pid:%d)\n", i, getpid());
        }
    }


    exit();

    return 0;
}
