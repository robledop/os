#ifndef KERNEL_H
#define KERNEL_H

void kernel_main();

#define ERROR(x) ((void *)(x))
#define ERROR_I(x) ((int)(x))
#define ISERR(x) ((int)(x) < 0)

#endif