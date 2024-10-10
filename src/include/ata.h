#ifndef ATA_H
#define ATA_H

int ata_read_sector(int lba, int total, void *buffer);
int ata_get_sector_size();

#endif