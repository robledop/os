#pragma once
#include <types.h>

#define ARP_REQUEST 1
#define ARP_REPLY 2


struct arp_header {
    uint16_t hw_type;
    uint16_t protocol_type;
    uint8_t hw_addr_len;
    uint8_t protocol_addr_len;
    uint16_t opcode;
    uint8_t sender_hw_addr[6];
    uint8_t sender_protocol_addr[4];
    uint8_t target_hw_addr[6];
    uint8_t target_protocol_addr[4];
};