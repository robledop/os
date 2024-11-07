#pragma once

#include <types.h>

uint16_t checksum(void *addr, int count, int start_sum);
unsigned int ip_to_int(const char *ip);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);
char *print_mac_address(uint8_t mac[6]);
