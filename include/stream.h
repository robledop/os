#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include "ata.h"

struct disk_stream {
    // Byte position in the disk
    uint32_t position;
    struct disk *disk;
};

struct disk_stream *disk_stream_create(int disk_index);
__attribute__((nonnull)) int disk_stream_seek(struct disk_stream *stream, uint32_t position);
__attribute__((nonnull)) int disk_stream_read(struct disk_stream *stream, void *out, uint32_t size);
__attribute__((nonnull)) void disk_stream_close(struct disk_stream *stream);
