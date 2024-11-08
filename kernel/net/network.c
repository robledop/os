#include <e1000.h>
#include <kernel.h>
#include <memory.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/ethernet.h>
#include <net/helpers.h>
#include <net/icmp.h>
#include <net/ipv4.h>
#include <net/network.h>
#include <net/udp.h>
#include <scheduler.h>
#include <types.h>
#include <vga_buffer.h>

bool network_ready       = false;
uint8_t my_ip_address[4] = {192, 168, 0, 96};
static uint8_t mac[6];

struct ether_type {
    uint16_t ether_type;
    char *name;
};

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

bool network_is_ready()
{
    return network_ready;
}

void network_set_my_ip_address(const uint8_t ip[4])
{
    memcpy(my_ip_address, ip, 4);
}

uint8_t *network_get_my_ip_address()
{
    return my_ip_address;
}

uint8_t *network_get_my_mac_address()
{
    return mac;
}

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

void network_receive(uint8_t *packet, const uint16_t len)
{
    const struct ether_header *ether_header = (struct ether_header *)packet;
    const uint16_t ether_type               = ntohs(ether_header->ether_type);

    switch (ether_type) {
    case ETHERTYPE_ARP:
        arp_receive(packet);
        break;
    case ETHERTYPE_IP:
        const struct ipv4_header *ipv4_header = (struct ipv4_header *)(packet + sizeof(struct ether_header));
        const uint8_t protocol                = ipv4_header->protocol;
        switch (protocol) {
        case IP_PROTOCOL_ICMP:
            if (network_compare_ip_addresses(ipv4_header->dest_ip, my_ip_address)) {
                icmp_receive(packet, len);
            }
            break;
        case IP_PROTOCOL_TCP:
            break;
        case IP_PROTOCOL_UDP:
            struct udp_header *udp_header =
                (struct udp_header *)(packet + sizeof(struct ether_header) + sizeof(struct ipv4_header));

            if (udp_header->dest_port == htons(DHCP_SOURCE_PORT)) {
                struct dhcp_header *dhcp_packet =
                    (struct dhcp_header *)(packet + sizeof(struct ether_header) + sizeof(struct ipv4_header) +
                                           sizeof(struct udp_header));
                if (dhcp_packet->op == DHCP_OP_OFFER && network_compare_mac_addresses(dhcp_packet->chaddr, mac)) {

                    kprintf("[ " KBOLD KGRN "OK" KRESET KWHT " ] ");
                    kprintf("IP address: %d.%d.%d.%d\n",
                            dhcp_packet->yiaddr[0],
                            dhcp_packet->yiaddr[1],
                            dhcp_packet->yiaddr[2],
                            dhcp_packet->yiaddr[3]);

                    // TODO: Send DHCP Ack and complete the DHCP process
                    network_set_my_ip_address(dhcp_packet->yiaddr);

                    network_ready = true;
                }
            }
            break;
        default:
        }
        break;
    default:
    }
}

int network_send_packet(const void *data, const uint16_t len)
{
    return e1000_send_packet(data, len);
}

bool network_compare_ip_addresses(const uint8_t ip1[], const uint8_t ip2[])
{
    for (int i = 0; i < 4; i++) {
        if (ip1[i] != ip2[i]) {
            return false;
        }
    }
    return true;
}

bool network_compare_mac_addresses(const uint8_t mac1[], const uint8_t mac2[])
{
    for (int i = 0; i < 6; i++) {
        if (mac1[i] != mac2[i]) {
            return false;
        }
    }
    return true;
}