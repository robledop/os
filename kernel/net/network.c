#include <e1000.h>
#include <kernel_heap.h>
#include <memory.h>
#include <net/arp.h>
#include <net/ethernet.h>
#include <net/helpers.h>
#include <net/icmp.h>
#include <net/ipv4.h>
#include <net/network.h>
#include <serial.h>
#include <types.h>

uint8_t my_ip_address[4] = {192, 168, 0, 96};
static uint8_t mac[6];

struct ether_type {
    uint16_t ether_type;
    char *name;
};

struct arp_packet {
    struct ether_header ether_header;
    struct arp_header arp_packet;
} __attribute__((packed));

struct icmp_packet {
    struct ether_header ether_header;
    struct ipv4_header ip_header;
    struct icmp_header icmp_header;
    uint8_t payload[];
} __attribute__((packed));

struct ether_type ether_types[] = {
    {ETHERTYPE_PUP,      "Xerox PUP"               },
    {ETHERTYPE_SPRITE,   "Sprite"                  },
    {ETHERTYPE_IP,       "IPv4"                    },
    {ETHERTYPE_ARP,      "ARP"                     },
    {ETHERTYPE_REVARP,   "Reverse ARP"             },
    {ETHERTYPE_AT,       "AppleTalk protocol"      },
    {ETHERTYPE_AARP,     "AppleTalk ARP"           },
    {ETHERTYPE_VLAN,     "IEEE 802.1Q VLAN tagging"},
    {ETHERTYPE_IPX,      "IPX"                     },
    {ETHERTYPE_IPV6,     "IPv6"                    },
    {ETHERTYPE_LOOPBACK, "Loopback"                }
};
void network_arp_receive(const struct ether_header *ether_header, const struct arp_header *arp);
bool network_compare_ip_addresses(uint8_t ip1[], uint8_t ip2[]);
void network_send_arp_reply(const struct ether_header *ether_header, const struct arp_header *arp);

const char *find_ether_type(const uint16_t ether_type)
{
    for (int i = 0; i < sizeof(ether_types) / sizeof(struct ether_type); i++) {
        if (ether_types[i].ether_type == ether_type) {
            return ether_types[i].name;
        }
    }
    return "Unknown";
}

void network_set_mac(const uint8_t *mac_addr)
{
    memcpy(mac, mac_addr, 6);
}

void network_parse_icmp_packet(const struct ether_header *ether_header, const struct ipv4_header *ipv4_header,
                               struct icmp_header *icmp_header, void *payload, uint16_t len)
{
    if (icmp_header->type == ICMP_V4_ECHO) {

        dbgprintf("Echo request: %d.%d.%d.%d\t MAC: %s\t icmp_seq: %d\n",
                  ipv4_header->source_ip[0],
                  ipv4_header->source_ip[1],
                  ipv4_header->source_ip[2],
                  ipv4_header->source_ip[3],
                  print_mac_address(ether_header->src_host),
                  icmp_header->sequence / 256);

        // Reply to the echo request
        struct icmp_packet *reply_packet = kzalloc(sizeof(struct icmp_packet));
        struct ether_header reply_ether_header;
        memcpy(reply_ether_header.dest_host, ether_header->src_host, 6);
        memcpy(reply_ether_header.src_host, mac, 6);
        reply_ether_header.ether_type = htons(ETHERTYPE_IP);

        reply_packet->ether_header = reply_ether_header;

        struct ipv4_header reply_ipv4_header;
        reply_ipv4_header.version               = 4;
        reply_ipv4_header.ihl                   = 0x05;
        reply_ipv4_header.dscp_ecn              = 0;
        reply_ipv4_header.total_length          = ntohs(len - sizeof(struct ether_header));
        reply_ipv4_header.flags_fragment_offset = 0x0;
        reply_ipv4_header.ttl                   = 64;
        reply_ipv4_header.protocol              = IP_PROTOCOL_ICMP;
        reply_ipv4_header.header_checksum       = 0;
        memcpy(reply_ipv4_header.source_ip, my_ip_address, 4);
        memcpy(reply_ipv4_header.dest_ip, ipv4_header->source_ip, 4);

        reply_ipv4_header.header_checksum = checksum((void *)&reply_ipv4_header, reply_ipv4_header.ihl * 4, 0);

        reply_packet->ip_header = reply_ipv4_header;

        icmp_header->type     = ICMP_REPLY;
        icmp_header->checksum = 0;
        icmp_header->checksum =
            checksum(icmp_header, len - sizeof(struct ether_header) - sizeof(struct ipv4_header), 0);

        struct icmp_header reply_icmp_header;
        memcpy(&reply_icmp_header, icmp_header, sizeof(struct icmp_header));

        reply_packet->icmp_header = reply_icmp_header;
        memcpy(reply_packet->payload,
               payload,
               len - sizeof(struct ether_header) - sizeof(struct ipv4_header) - sizeof(struct icmp_header));

        dbgprintf("Echo reply: %d.%d.%d.%d\t MAC: %s\t icmp_seq: %d\n",
                  reply_packet->ip_header.dest_ip[0],
                  reply_packet->ip_header.dest_ip[1],
                  reply_packet->ip_header.dest_ip[2],
                  reply_packet->ip_header.dest_ip[3],
                  print_mac_address(reply_packet->ether_header.dest_host),
                  reply_packet->icmp_header.sequence / 256);

        network_send_packet(reply_packet, len);
    }
}

void network_receive(uint8_t *data, const uint16_t len)
{
    const struct ether_header *ether_header = (struct ether_header *)data;
    const uint16_t ether_type               = ntohs(ether_header->ether_type);

    switch (ether_type) {
    case ETHERTYPE_ARP:
        const struct arp_header *arp = (struct arp_header *)(data + sizeof(struct ether_header));
        network_arp_receive(ether_header, arp);
        break;
    case ETHERTYPE_IP:
        const struct ipv4_header *ipv4_header = (struct ipv4_header *)(data + sizeof(struct ether_header));
        const uint8_t protocol                = ipv4_header->protocol;
        switch (protocol) {
        case IP_PROTOCOL_ICMP:
            auto const icmp = (struct icmp_header *)(data + sizeof(struct ether_header) + sizeof(struct ipv4_header));
            void *payload =
                (void *)(data + sizeof(struct ether_header) + sizeof(struct ipv4_header) + sizeof(struct icmp_header));
            network_parse_icmp_packet(ether_header, ipv4_header, icmp, payload, len);
            break;
        case IP_PROTOCOL_TCP:
            break;
        case IP_PROTOCOL_UDP:
            break;
        default:
        }
        break;
    case ETHERTYPE_REVARP:
        break;
    default:
    }
}

int network_send_packet(const void *data, const uint16_t len)
{
    return e1000_send_packet(data, len);
}

void network_arp_receive(const struct ether_header *ether_header, const struct arp_header *arp)
{
    const uint16_t opcode = ntohs(arp->opcode);
    switch (opcode) {
    case ARP_REQUEST:
        if (network_compare_ip_addresses(arp->target_protocol_addr, my_ip_address)) {
            network_send_arp_reply(ether_header, arp);
        }
        break;
    case ARP_REPLY:
        break;
    default:
        break;
    }
}

bool network_compare_ip_addresses(uint8_t ip1[], uint8_t ip2[])
{
    for (int i = 0; i < 4; i++) {
        if (ip1[i] != ip2[i]) {
            return false;
        }
    }
    return true;
}

void network_send_arp_reply(const struct ether_header *ether_header, const struct arp_header *arp)
{
    struct ether_header reply_ether_header;
    memcpy(reply_ether_header.dest_host, ether_header->src_host, 6);
    memcpy(reply_ether_header.src_host, mac, 6);
    reply_ether_header.ether_type = htons(ETHERTYPE_ARP);

    struct arp_header reply_arp_header;
    reply_arp_header.hw_type           = arp->hw_type;
    reply_arp_header.protocol_type     = arp->protocol_type;
    reply_arp_header.hw_addr_len       = arp->hw_addr_len;
    reply_arp_header.protocol_addr_len = arp->protocol_addr_len;
    reply_arp_header.opcode            = htons(ARP_REPLY);
    memcpy(reply_arp_header.sender_hw_addr, mac, 6);
    memcpy(reply_arp_header.sender_protocol_addr, my_ip_address, 4);
    memcpy(reply_arp_header.target_hw_addr, arp->sender_hw_addr, 6);
    memcpy(reply_arp_header.target_protocol_addr, arp->sender_protocol_addr, 4);

    struct arp_packet *reply_packet = kzalloc(sizeof(struct arp_packet));

    reply_packet->ether_header = reply_ether_header;
    reply_packet->arp_packet   = reply_arp_header;

    dbgprintf("ARP reply. Destination IP: %d.%d.%d.%d\n",
              reply_packet->arp_packet.target_protocol_addr[0],
              reply_packet->arp_packet.target_protocol_addr[1],
              reply_packet->arp_packet.target_protocol_addr[2],
              reply_packet->arp_packet.target_protocol_addr[3]);

    network_send_packet(reply_packet, sizeof(struct arp_packet));
    kfree(reply_packet);
}