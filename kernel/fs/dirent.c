#include <dirent.h>
#include <stddef.h>

unsigned short dirent_record_length(unsigned short name_length)
{
    unsigned short reclen = offsetof(struct dirent, name) + name_length + 1; // +1 for null terminator

    // Align to 8-byte boundary
    reclen = (reclen + 7) & ~7;
    return reclen;
}