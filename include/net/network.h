#pragma once
#include <types.h>

void network_receive(uint8_t *data, uint16_t len);
int network_send_packet(const void *data, uint16_t len);
void network_set_mac(const uint8_t *mac_addr);
