#include <net/helpers.h>
#include <string.h>
#include <vga_buffer.h>

// https://www.lemoda.net/c/ip-to-integer/

#define INVALID 0

/// @brief Convert the character string in "ip" into an unsigned integer.
/// This assumes that an unsigned integer contains at least 32 bits.
unsigned int ip_to_int(const char *ip)
{
    unsigned v        = 0;
    const char *start = ip;

    for (int i = 0; i < 4; i++) {
        int n = 0;
        while (1) {
            const char c = *start;
            start++;
            if (c >= '0' && c <= '9') {
                n *= 10;
                n += c - '0';
            } else if ((i < 3 && c == '.') || i == 3) {
                break;
            } else {
                return INVALID;
            }
        }
        if (n >= 256) {
            return INVALID;
        }
        v *= 256;
        v += n;
    }
    return v;
}

/// @brief Computes the checksum according to the algorithm described in RFC 1071.
/// @code
/// checksum(header, header->ihl * 4, 0);
/// @endcode
uint16_t checksum(void *addr, int count, const int start_sum)
{
    register uint32_t sum = start_sum;

    uint16_t *ptr = addr;

    while (count > 1) {
        sum += *ptr++;
        count -= 2;
    }

    if (count > 0) {
        sum += *(uint8_t *)ptr;
    }

    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

/// @brief Converts the unsigned short integer netshort from network byte order to host byte order.
uint16_t ntohs(const uint16_t data)
{
    return ((data & 0x00ff) << 8) | (data & 0xff00) >> 8;
}

/// @brief Converts the unsigned short integer hostshort from host byte order to network byte order.
uint16_t htons(const uint16_t data)
{
    return ((data & 0x00ff) << 8) | (data & 0xff00) >> 8;
}

// ReSharper disable once CppDFAConstantFunctionResult
char *print_mac_address(uint8_t mac[6])
{
    // ReSharper disable once CppUseAuto
    char *result = "00:00:00:00:00:00";
    for (int i = 0; i < 6; i++) {
        result[i * 3]     = "0123456789ABCDEF"[mac[i] / 16];
        result[i * 3 + 1] = "0123456789ABCDEF"[mac[i] % 16];
    }

    return result;
}
