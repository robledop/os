#include "stdio.h"

// Fibonacci recursive
uint64_t fib(uint64_t n)
{
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main(const int argc, char **argv)
{
    putchar('\n');
    printf("Calculating fibonacci of 42 using recursion.\n");
    const uint64_t result = fib(42);
    printf("Result: %d", result);

    // uint64_t a = 0, b = 1, c, i, n = 100;
    // for (i = 0; i < n; i++) {
    //     if (i <= 1) {
    //         c = i;
    //     } else {
    //         c = a + b;
    //         a = b;
    //         b = c;
    //     }
    //     printf("%d ", c);
    // }

    return 0;
}
