#pragma once

#include <stdint.h>


#define S_IRUSR 0x100 // read permission, owner
#define S_IWUSR 0x80  // write permission, owner
#define S_IXUSR 0x40  // execute/search permission, owner
#define S_IRGRP 0x20  // read permission, group
#define S_IWGRP 0x10  // write permission, group
#define S_IXGRP 0x8   // execute/search permission, group
#define S_IROTH 0x4   // read permission, others
#define S_IWOTH 0x2   // write permission, others
#define S_IXOTH 0x1   // execute/search permission, others

#define S_IFMT 0xF000   // type of file
#define S_IFSOCK 0xC000 // socket
#define S_IFLNK 0xA000  // symbolic link
#define S_IFREG 0x8000  // regular file
#define S_IFBLK 0x6000  // block device
#define S_IFDIR 0x4000  // directory
#define S_IFCHR 0x2000  // character device
#define S_IFIFO 0x1000  // fifo

#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR) // is directory

struct stat {
    uint16_t st_dev;     // ID of device containing file
    uint16_t st_ino;     // inode number
    uint16_t st_mode;    // protection
    uint16_t st_nlink;   // number of hard links
    uint16_t st_uid;     // user ID of owner
    uint16_t st_gid;     // group ID of owner
    uint16_t st_rdev;    // device ID (if special file)
    uint32_t st_size;    // total size, in bytes
    uint32_t st_blksize; // block size for file system I/O
    uint32_t st_blocks;  // number of 512B blocks allocated
    uint32_t st_atime;   // time of last access
    uint32_t st_mtime;   // time of last modification
    uint32_t st_ctime;   // time of last status change
    bool st_lfn;         // long file name
};
