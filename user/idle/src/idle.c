#include <syscall.h>

int main(const int argc, char **argv)
{
    while (1) {
        syscall0(SYSCALL_IDLE);
    }

    return 0;
}