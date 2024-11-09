#include <kernel_heap.h>
#include <memory.h>
#include <net/dhcp.h>
#include <net/helpers.h>
#include <net/network.h>
#include <vga_buffer.h>

void set_dhcp_options(uint8_t *options)
{
    int offset = 0;

    // Option 53: DHCP Message Type
    options[offset++] = DHCP_OPTION_MESSAGE_TYPE;     // Type
    options[offset++] = DHCP_OPTION_MESSAGE_TYPE_LEN; // Length
    options[offset++] = DHCP_DISCOVER;                // Value (1 for Discover)

    // Option 55: Parameter Request List
    options[offset++] = DHCP_OPTION_PARAMETER_REQUEST; // Type
    options[offset++] = 2;                             // Length (requesting 2 parameters)
    options[offset++] = 0x01;                          // Request Subnet Mask
    options[offset++] = 0x03;                          // Request Default Gateway (Router)

    // Option 255: End of Options
    options[offset++] = 0xFF; // End of options
}

void dhcp_send_request(uint8_t mac[6])
{
    kprintf("[ " KBOLD KGRN "OK" KRESET KWHT " ] ");
    kprintf("Sending DHCP Discover...\n");
    struct ether_header ether_header = {
        .dest_host  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        .ether_type = htons(ETHERTYPE_IP),
    };
    memcpy(ether_header.src_host, mac, 6);

    struct ipv4_header ipv4_header = {
        .ihl            = 0x05,
        .version        = 4,
        .dscp_ecn       = 0x00,
        .total_length   = htons(sizeof(struct ipv4_header) + sizeof(struct udp_header) + sizeof(struct dhcp_header)),
        .identification = 0x0000,
        .flags_fragment_offset = 0x0000,
        .ttl                   = 0x40,
        .protocol              = IP_PROTOCOL_UDP,
        .header_checksum       = 0x0000,
        .source_ip             = {0x00, 0x00, 0x00, 0x00},
        .dest_ip               = {0xFF, 0xFF, 0xFF, 0xFF},
    };

    ipv4_header.header_checksum = checksum(&ipv4_header, ipv4_header.ihl * 4, 0);

    struct udp_header udp_header = {
        .src_port  = htons(DHCP_SOURCE_PORT),
        .dest_port = htons(DHCP_DEST_PORT),
        .len       = htons(sizeof(struct udp_header) + sizeof(struct dhcp_header)),
        .checksum  = 0x0000,
    };

    struct dhcp_header dhcp_header = {
        .op     = DHCP_OP_DISCOVER,
        .htype  = DHCP_HTYPE_ETH,
        .hlen   = DHCP_HLEN_ETH,
        .hops   = 0,
        .xid    = 0x726f626,
        .secs   = 0x0000,
        .flags  = htons(DHCP_FLAG_BROADCAST),
        .ciaddr = 0,
        .yiaddr = 0,
        .siaddr = 0,
        .giaddr = 0,
        .magic  = htonl(DHCP_MAGIC_COOKIE),
    };

    memcpy(dhcp_header.chaddr, mac, 6);

    set_dhcp_options(dhcp_header.options);

    // Build the UDP pseudo-header
    struct udp_pseudo_header pseudo_header = {
        .src_ip     = {ipv4_header.source_ip[0],
                       ipv4_header.source_ip[1],
                       ipv4_header.source_ip[2],
                       ipv4_header.source_ip[3]                                                                        },
        .dest_ip    = {ipv4_header.dest_ip[0],   ipv4_header.dest_ip[1], ipv4_header.dest_ip[2], ipv4_header.dest_ip[3]},
        .zero       = 0,
        .protocol   = IP_PROTOCOL_UDP,
        .udp_length = udp_header.len,
    };

    // Buffer to store pseudo-header + UDP header + DHCP data
    uint8_t udp_checksum_buf[sizeof(struct udp_pseudo_header) + sizeof(struct udp_header) + sizeof(struct dhcp_header)];

    // Copy pseudo-header, UDP header, and DHCP data to the buffer
    memcpy(udp_checksum_buf, &pseudo_header, sizeof(struct udp_pseudo_header));
    memcpy(udp_checksum_buf + sizeof(struct udp_pseudo_header), &udp_header, sizeof(struct udp_header));
    memcpy(udp_checksum_buf + sizeof(struct udp_pseudo_header) + sizeof(struct udp_header),
           &dhcp_header,
           sizeof(struct dhcp_header));

    // Calculate the UDP checksum with the pseudo-header
    udp_header.checksum = checksum(udp_checksum_buf, sizeof(udp_checksum_buf), 0);

    struct dhcp_packet packet = {
        .eth  = ether_header,
        .ip   = ipv4_header,
        .udp  = udp_header,
        .dhcp = dhcp_header,
    };

    network_send_packet(&packet, sizeof(struct dhcp_packet));
}