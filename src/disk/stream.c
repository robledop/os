#include "stream.h"
#include <stdbool.h>
#include "config.h"
#include "kernel_heap.h"
#include "serial.h"
#include "assert.h"
#include "memory.h"
#include "disk.h"

struct disk_stream *disk_stream_create(int disk_index)
{
    dbgprintf("Creating disk stream for disk %d\n", disk_index);
    struct disk *disk = disk_get(disk_index);
    if (!disk)
    {
        warningf("Failed to get disk %d\n", disk_index);
        return NULL;
    }

    struct disk_stream *stream = kzalloc(sizeof(struct disk_stream));
    stream->position = 0;
    stream->disk = disk;
    return stream;
}

int disk_stream_seek(struct disk_stream *stream, int position)
{
    stream->position = position;
    return 0;
}

int disk_stream_read(struct disk_stream *stream, void *out, int size)
{
    ASSERT(stream->disk->sector_size > 0, "Invalid sector size");
    // ASSERT(size > 0, "Invalid size");
    dbgprintf("Reading %d bytes from disk stream\n", size);
    int sector = stream->position / stream->disk->sector_size;
    int offset = stream->position % stream->disk->sector_size;
    int to_read = size;
    bool overflow = (offset + to_read) >= stream->disk->sector_size;
    char buffer[stream->disk->sector_size];
    // memset(buffer, 0, stream->disk->sector_size);

    if (overflow)
    {
        to_read -= (offset + to_read) - stream->disk->sector_size;
    }

    int res = disk_read_block(stream->disk, sector, 1, buffer);
    if (res < 0)
    {
        warningf("Failed to read block\n");
        return res;
    }

    for (int i = 0; i < to_read; i++)
    {
        *(char *)out = buffer[offset + i];
        out = (char *)out + 1;
        // *(char *)out++ = buffer[offset + i];
    }

    stream->position += to_read;
    if (overflow)
    {
        dbgprintf("Overflow, reading more\n");
        res = disk_stream_read(stream, out, size - to_read);
    }

    dbgprintf("Read %d bytes. Value: %x\n", size, buffer);

    ASSERT((uint16_t *)out != 0, "Invalid out pointer");

    return res;
}

void disk_stream_close(struct disk_stream *stream) { kfree(stream); }
