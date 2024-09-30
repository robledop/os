#ifndef STREAMER_H
#define STREAMER_H

#include "disk.h"

struct disk_stream
{
    // Byte position in the disk
    int position;
    struct disk *disk;
};

struct disk_stream *disk_stream_create(int disk_index);
int disk_stream_seek(struct disk_stream *stream, int position);
int disk_stream_read(struct disk_stream *stream, void *out, int size);
void disk_stream_close(struct disk_stream *stream);

#endif