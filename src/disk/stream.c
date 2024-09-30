#include "stream.h"
#include "config.h"
#include "kheap.h"

struct disk_stream *disk_stream_create(int disk_index)
{
    struct disk *disk = disk_get(disk_index);
    if (!disk)
    {
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
    int sector = stream->position / SECTOR_SIZE;
    int offset = stream->position % SECTOR_SIZE;
    char buffer[SECTOR_SIZE];

    int res = disk_read_block(stream->disk, sector, 1, buffer);
    if (res < 0)
    {
        goto out;
    }

    int to_read = size > SECTOR_SIZE ? SECTOR_SIZE : size;
    for (int i = 0; i < to_read; i++)
    {
        *(char *)out++ = buffer[offset + i];
    }

    stream->position += to_read;
    if (size > SECTOR_SIZE)
    {
        res = disk_stream_read(stream, out, size - SECTOR_SIZE);
    }

out:
    return res;
}

void disk_stream_close(struct disk_stream *stream)
{
    kfree(stream);
}