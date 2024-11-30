#include <debug.h>
#include <disk.h>
#include <kernel_heap.h>
#include <serial.h>
#include <stream.h>

struct disk_stream *disk_stream_create(const int disk_index)
{
    struct disk *disk = disk_get(disk_index);
    if (!disk) {
        panic("Failed to get disk %d\n");
        return nullptr;
    }

    struct disk_stream *stream = kzalloc(sizeof(struct disk_stream));
    if (!stream) {
        panic("Failed to allocate memory for disk stream\n");
        return nullptr;
    }
    stream->position = 0;
    stream->disk     = disk;
    return stream;
}

int disk_stream_seek(struct disk_stream *stream, const uint32_t position)
{
    stream->position = position;
    return 0;
}

int disk_stream_read(struct disk_stream *stream, void *out, const uint32_t size)
{ // NOLINT(*-no-recursion)
    ASSERT(stream->disk->sector_size > 0, "Invalid sector size");
    const uint32_t sector = stream->position / stream->disk->sector_size;
    const uint32_t offset = stream->position % stream->disk->sector_size;
    uint32_t to_read      = size;
    const bool overflow   = (offset + to_read) >= stream->disk->sector_size;
    uint8_t buffer[stream->disk->sector_size];

    if (overflow) {
        to_read -= (offset + to_read) - stream->disk->sector_size;
    }

    int res = disk_read_sector(sector, buffer);
    if (res < 0) {
        panic("Failed to read block\n");
        return res;
    }

    for (uint32_t i = 0; i < to_read; i++) {
        *(uint8_t *)out = buffer[offset + i];
        out             = (uint8_t *)out + 1;
    }

    stream->position += to_read;
    if (overflow) {
        res = disk_stream_read(stream, out, size - to_read);
    }

    ASSERT((uint16_t *)out != nullptr, "Invalid out pointer");

    return res;
}

int disk_stream_write(struct disk_stream *stream, const void *in, const uint32_t size)
{
    ASSERT(stream->disk->sector_size > 0, "Invalid sector size");
    const uint32_t sector = stream->position / stream->disk->sector_size;
    const uint32_t offset = stream->position % stream->disk->sector_size;
    uint32_t to_write     = size;
    const bool overflow   = (offset + to_write) >= stream->disk->sector_size;
    uint8_t buffer[stream->disk->sector_size];

    if (overflow) {
        to_write -= (offset + to_write) - stream->disk->sector_size;
    }

    int res = disk_read_sector(sector, buffer);
    if (res < 0) {
        warningf("Failed to read block\n");
        return res;
    }

    for (uint32_t i = 0; i < to_write; i++) {
        buffer[offset + i] = *(uint8_t *)in;
        in                 = (uint8_t *)in + 1;
    }

    res = disk_write_sector(sector, buffer);
    if (res < 0) {
        warningf("Failed to write block\n");
        return res;
    }

    stream->position += to_write;
    if (overflow) {
        res = disk_stream_write(stream, in, size - to_write);
    }

    return res;
}

void disk_stream_close(struct disk_stream *stream)
{
    kfree(stream);
}
