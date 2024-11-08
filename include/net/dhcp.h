#pragma once

#include <net/ethernet.h>
#include <net/ipv4.h>
#include <net/udp.h>
#include <types.h>

#define DHCP_CHADDR_LEN 6
#define DHCP_SNAME_LEN 64
#define DHCP_FILE_LEN 128
#define DHCP_OPTIONS_LEN 128

#define DHCP_OP_DISCOVER 0x01
#define DHCP_OP_OFFER 0x02
#define DHCP_SOURCE_PORT 68
#define DHCP_DEST_PORT 67

#define DHCP_HTYPE_ETH 0x01
#define DHCP_HLEN_ETH 0x06

#define DHCP_MAGIC_COOKIE 0x63538263

#define DHCP_FLAG_BROADCAST 0x8000
#define DHCP_FLAG_UNICAST 0x0000

#define DHCP_OPTION_MESSAGE_TYPE 53
#define DHCP_OPTION_PARAMETER_REQUEST 55
#define DHCP_OPTION_MESSAGE_TYPE_LEN 1
#define DHCP_DISCOVER 1

struct dhcp_header {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint8_t ciaddr[4];               // Client IP address (only filled if client is already bound)
    uint8_t yiaddr[4];               // 'Your' (client) IP address
    uint8_t siaddr[4];               // IP address of next server to use in bootstrap
    uint8_t giaddr[4];               // Relay agent IP address
    uint8_t chaddr[DHCP_CHADDR_LEN]; // Client hardware address
    uint8_t reserved[10];
    uint8_t sname[DHCP_SNAME_LEN];
    uint8_t file[DHCP_FILE_LEN];
    uint32_t magic;
    uint8_t options[DHCP_OPTIONS_LEN];
} __attribute__((packed));

struct dhcp_packet {
    struct ether_header eth;
    struct ipv4_header ip;
    struct udp_header udp;
    struct dhcp_header dhcp;
} __attribute__((packed));

void dhcp_send_request(uint8_t mac[6]);