#pragma once

#include "types.h"

int ata_read_sector(uint32_t lba, int total, void *buffer);
int ata_get_sector_size();
