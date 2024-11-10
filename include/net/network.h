#pragma once
#include <types.h>

void network_receive(uint8_t *packet, uint16_t len);
int network_send_packet(const void *data, uint16_t len);
void network_set_mac(const uint8_t *mac_addr);
uint8_t *network_get_my_ip_address();
bool network_compare_ip_addresses(const uint8_t ip1[], const uint8_t ip2[]);
bool network_compare_mac_addresses(const uint8_t mac1[], const uint8_t mac2[]);
uint8_t *network_get_my_mac_address();
bool network_is_ready();

void network_set_dns_servers(uint32_t dns_servers_p[], size_t dns_server_count);
void network_set_my_ip_address(const uint8_t ip[4]);
void network_set_subnet_mask(const uint8_t ip[4]);
void network_set_default_gateway(const uint8_t ip[4]);
void network_set_state(bool state);