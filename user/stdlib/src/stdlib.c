#include "stdlib.h"
#include "os.h"
#include <stddef.h>

void *malloc(size_t size)
{
    return os_malloc(size);
}

void free(void *ptr)
{
    os_free(ptr);
}

