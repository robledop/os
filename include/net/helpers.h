#pragma once
#include <stdint.h>


uint16_t checksum(void *addr, int count, int start_sum);

unsigned int ip_to_int(const char ip[static 1]);
void int_to_ip(uint32_t ip_addr, char *result[static 16]);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);
uint32_t htonl(uint32_t data);
char *get_mac_address_string(uint8_t mac[static 6]);

uint8_t read8(uint64_t addr);
uint16_t read16(uint64_t addr);
uint32_t read32(uint64_t addr);
uint64_t read64(uint64_t addr);
void write8(uint64_t addr, uint8_t data);
void write16(uint64_t addr, uint16_t data);
void write32(uint64_t addr, uint32_t data);
void write64(uint64_t addr, uint64_t data);
