#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <stdint.h>

int ata_read_sector(uint32_t lba, int total, void *buffer);
__attribute__((nonnull)) int ata_write_sector(uint32_t lba, int total, void *buffer);
int ata_get_sector_size(void);
