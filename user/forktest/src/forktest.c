#include <string.h>


#include "stdio.h"
#include "stdlib.h"

int main(const int argc, char **argv)
{
    printf(KBOLD KBLU "\nTests with waitpid()");
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
        printf("\n create_process: %d", i);
        const int pid = create_process((char *)"echo lalala", current_directory);
        if (pid < 0) {
        } else {
            waitpid(pid, nullptr);
            // wait(nullptr);
        }
    }

    for (int i = 0; i < 10; i++) {
        const int r = fork();
        if (r < 0) {
            printf("Fork failed\n");
        } else if (r == 0) {
            printf(KGRN "\nForked child %d (pid:%d)" KWHT, i, getpid());
            exit();
        } else {
            wait(nullptr);
            printf(KYEL "\nParent of %d (pid:%d)" KWHT, i, getpid());
        }
    }

    printf(KBOLD KBLU "\nTests without waitpid()\n" KRESET);

    const int nowait = fork();

    if (nowait < 0) {
        printf("Fork failed\n");
    } else if (nowait == 0) {
        printf(KCYN "Child (pid:%d)\n", getpid());
        printf("Child will exec blank.elf" KWHT " ");
        exec("0:/bin/blank.elf", nullptr);

        printf("This should not be printed\n");
    } else {
        // waitpid(rc, nullptr);
        printf(KYEL "\nAfter forking. Parent of %d (pid:%d)", rc, getpid());
    }

    for (int i = 0; i < 10; i++) {
        const char *current_directory = get_current_directory();
        printf("\n create_process: %d", i);
        const int pid = create_process((char *)"echo lalala", current_directory);
        if (pid < 0) {
        } else {
            // waitpid(pid, nullptr);
        }
    }

    for (int i = 0; i < 10; i++) {
        const int r = fork();
        if (r < 0) {
            printf("Fork failed\n");
        } else if (r == 0) {
            printf(KGRN "\nForked child %d (pid:%d)" KWHT, i, getpid());
            exit();
        } else {
            // waitpid(r, nullptr);
            printf(KYEL "\nParent of %d (pid:%d)" KWHT, i, getpid());
        }
    }


    printf(KBOLD KBLU "\nTests with wait()\n" KRESET);

    const int waitp = fork();

    if (waitp < 0) {
        printf("Fork failed\n");
    } else if (waitp == 0) {
        printf(KCYN "Child (pid:%d)\n", getpid());
        printf("Child will exec blank.elf" KWHT " ");
        exec("0:/bin/blank.elf", nullptr);

        printf("This should not be printed\n");
    } else {
        wait(nullptr);
        printf(KYEL "\nAfter forking. Parent of %d (pid:%d)", rc, getpid());
    }

    for (int i = 0; i < 10; i++) {
        const char *current_directory = get_current_directory();
        printf("\n create_process: %d", i);
        const int pid = create_process((char *)"echo lalala", current_directory);
        if (pid < 0) {
        } else {
            wait(nullptr);
        }
    }

    for (int i = 0; i < 10; i++) {
        const int r = fork();
        if (r < 0) {
            printf("Fork failed\n");
        } else if (r == 0) {
            printf(KGRN "\nForked child %d (pid:%d)" KWHT, i, getpid());
            exit();
        } else {
            wait(nullptr);
            printf(KYEL "\nParent of %d (pid:%d)" KWHT, i, getpid());
        }
    }

    exit();

    return 0;
}
