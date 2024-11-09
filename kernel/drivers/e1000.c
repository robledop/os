#include <e1000.h>
#include <idt.h>
#include <io.h>
#include <kernel_heap.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/helpers.h>
#include <net/network.h>
#include <serial.h>
#include <vga_buffer.h>

#define IRQ0 0x20


void e1000_receive();
bool e1000_start();
void e1000_linkup();
int e1000_send_packet(const void *data, uint16_t len);
void e1000_receive_packets();

static uint8_t bar_type;                                   // Type of BAR0
static uint16_t io_base;                                   // IO Base Address
static uint64_t mem_base;                                  // MMIO Base Address
static bool eeprom_exists;                                 // A flag indicating if eeprom exists
static uint8_t mac[6];                                     // A buffer for storing the mack address
static struct e1000_rx_desc *rx_descs[E1000_RX_RING_SIZE]; // Receive Descriptor Buffers
static struct e1000_tx_desc *tx_descs[E1000_TX_RING_SIZE]; // Transmit Descriptor Buffers
static uint16_t rx_cur;                                    // Current Receive Descriptor Buffer
static uint16_t tx_cur;                                    // Current Transmit Descriptor Buffer
static struct pci_device *pci_device;


void e1000_write_command(const uint16_t p_address, const uint32_t p_value)
{
    if (bar_type == PCI_BAR_MEM) {
        write32(mem_base + p_address, p_value);
    } else {
        outl(io_base, p_address);
        outl(io_base + 4, p_value);
    }
}

uint32_t e1000_read_command(const uint16_t p_address)
{
    if (bar_type == PCI_BAR_MEM) {
        return read32(mem_base + p_address);
    }

    outl(io_base, p_address);
    return inl(io_base + 4);
}

bool e1000_detect_eeprom()
{
    uint32_t val = 0;
    e1000_write_command(REG_EEPROM, 0x1);

    for (int i = 0; i < 1000 && !eeprom_exists; i++) {
        val = e1000_read_command(REG_EEPROM);
        if (val & 0x10) {
            eeprom_exists = true;
        } else {
            eeprom_exists = false;
        }
    }
    return eeprom_exists;
}

uint32_t e1000_eeprom_read(const uint8_t addr)
{
    uint16_t data = 0;
    uint32_t tmp  = 0;
    if (eeprom_exists) {
        e1000_write_command(REG_EEPROM, (1) | ((uint32_t)(addr) << 8));
        while (!((tmp = e1000_read_command(REG_EEPROM)) & (1 << 4)))
            ;
    } else {
        e1000_write_command(REG_EEPROM, (1) | ((uint32_t)(addr) << 2));
        while (!((tmp = e1000_read_command(REG_EEPROM)) & (1 << 1)))
            ;
    }
    data = (uint16_t)((tmp >> 16) & 0xFFFF);
    return data;
}

bool e1000_read_mac_address()
{
    if (eeprom_exists) {
        uint32_t temp = e1000_eeprom_read(0);
        mac[0]        = temp & 0xff;
        mac[1]        = temp >> 8;
        temp          = e1000_eeprom_read(1);
        mac[2]        = temp & 0xff;
        mac[3]        = temp >> 8;
        temp          = e1000_eeprom_read(2);
        mac[4]        = temp & 0xff;
        mac[5]        = temp >> 8;
    } else {
        const uint8_t *mem_base_mac_8   = (uint8_t *)(mem_base + 0x5400);
        const uint32_t *mem_base_mac_32 = (uint32_t *)(mem_base + 0x5400);
        if (mem_base_mac_32[0] != 0) {
            for (int i = 0; i < 6; i++) {
                mac[i] = mem_base_mac_8[i];
            }
        } else {
            return false;
        }
    }

    network_set_mac(mac);
    return true;
}

void e1000_rx_init()
{
    // ReSharper disable once CppDFAMemoryLeak
    const uint8_t *ptr = (uint8_t *)(kmalloc(sizeof(struct e1000_rx_desc) * E1000_RX_RING_SIZE + 16));

    const struct e1000_rx_desc *descs = (struct e1000_rx_desc *)ptr;

    for (int i = 0; i < E1000_RX_RING_SIZE; i++) {
        rx_descs[i]         = (struct e1000_rx_desc *)((uint8_t *)descs + i * 16);
        rx_descs[i]->addr   = (uint64_t)(uint8_t *)(kmalloc(8192 + 16));
        rx_descs[i]->status = 0;
    }

    e1000_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32));
    e1000_write_command(REG_TXDESCHI, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

    e1000_write_command(REG_RXDESCLO, (uint64_t)ptr);
    e1000_write_command(REG_RXDESCHI, 0);

    e1000_write_command(REG_RXDESCLEN, E1000_RX_RING_SIZE * 16);

    e1000_write_command(REG_RXDESCHEAD, 0);
    e1000_write_command(REG_RXDESCTAIL, E1000_RX_RING_SIZE - 1);
    rx_cur = 0;
    e1000_write_command(REG_RCTRL,
                        RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC |
                            RCTL_BSIZE_8192);
}

void e1000_tx_init()
{
    // ReSharper disable once CppDFAMemoryLeak
    const uint8_t *ptr = (uint8_t *)(kmalloc(sizeof(struct e1000_tx_desc) * E1000_TX_RING_SIZE + 16));

    const struct e1000_tx_desc *descs = (struct e1000_tx_desc *)ptr;

    for (int i = 0; i < E1000_TX_RING_SIZE; i++) {
        tx_descs[i]         = (struct e1000_tx_desc *)((uint8_t *)descs + i * 16);
        tx_descs[i]->addr   = 0;
        tx_descs[i]->cmd    = 0;
        tx_descs[i]->status = TSTA_DD;
    }

    e1000_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32));
    e1000_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

    // now setup total length of descriptors
    e1000_write_command(REG_TXDESCLEN, E1000_TX_RING_SIZE * 16);

    // setup numbers
    e1000_write_command(REG_TXDESCHEAD, 0);
    e1000_write_command(REG_TXDESCTAIL, 0);
    tx_cur = 0;
    e1000_write_command(REG_TCTRL, TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT) | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);

    // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000
    // cards, but for the e1000e cards you should set the TCTRL register as follows. For detailed description of each
    // bit, please refer to the Intel Manual. In the case of I217 and 82577LM packets will not be sent if the TCTRL is
    // not configured using the following bits.
    // e1000_write_command(REG_TCTRL, 0b0110000000000111111000011111010);
    // e1000_write_command(REG_TIPG, 0x0060200A);
}

void e1000_enable_interrupt()
{
    e1000_write_command(REG_IMASK, 0x1F6DC); // Enable RX and TX interrupts

    // Clear specific bits to ensure unwanted interrupts are not triggered
    e1000_write_command(REG_IMASK, 0xff & ~4);

    // Clear any pending interrupts by reading ICR (Interrupt Cause Read) register
    e1000_read_command(0xc0);
}

void e1000_init(struct pci_device *pci)
{
    pci_device = pci;
    bar_type   = pci_get_bar(pci_device, PCI_BAR_MEM) & 1;
    io_base    = pci_get_bar(pci_device, PCI_BAR_IO) & ~1;
    mem_base   = pci_get_bar(pci_device, PCI_BAR_MEM) & ~3;

    pci_enable_bus_mastering(pci);
    eeprom_exists = false;
    if (e1000_start()) {
        kprintf("[ " KBOLD KGRN "OK" KRESET KWHT " ] E1000 started\n");
    }

    arp_init();
}

// Set the Receive Delay Timer to define the RXDMT0 threshold
// e1000_write_command(REG_RDTR, desired_threshold_value);

// Set the Interrupt Throttling Rate (in microseconds)
// e1000_write_command(REG_ITR, desired_itr_value);

// Enable LSC, RXDMT0, and RXT0 interrupts
// e1000_write_command(REG_IMS, E1000_IMS_LSC | E1000_IMS_RXDMT0 | E1000_IMS_RXT0);

void e1000_interrupt_handler(int interrupt, const struct interrupt_frame *frame)
{
    if (interrupt == pci_device->header.irq + IRQ0) {
        // This might be needed here if your handler doesn't clear interrupts from each device and must be done
        // before EOI if using the PIC. Without this, the card will spam interrupts as the int-line will stay high.
        e1000_write_command(REG_IMASK, 0x1);

        // - Bit 0 (0x01): Transmit Descriptor Written Back (TXDW) - Indicates that the transmit descriptor has been
        // written back.
        // - Bit 1 (0x02): Transmit Queue Empty (TXQE) - Indicates that the transmit queue is empty.
        // - Bit 2 (0x04): Link Status Change (LSC) - Indicates a change in the link status.
        // - Bit 3 (0x08): Receive Descriptor Minimum Threshold (RXDMT0) - Indicates that the receive descriptor
        // minimum threshold has been reached.
        // - Bit 4 (0x10): Good Threshold (GPI) - Indicates a good threshold event.
        // - Bit 6 (0x40): Receive Timer Interrupt (RXT0) - Indicates that the receive timer has expired.
        // - Bit 7 (0x80): Receive Descriptor Written Back (RXO) - Indicates that a receive descriptor has been
        // written back.
        const uint32_t status = e1000_read_command(REG_ICR);
        if (status & E1000_ICR_LSC) {
            e1000_linkup();
        } else if (status & 0x10) {
            // good threshold
        } else if (status & 0x80) {
            e1000_receive();
        }
    }
}

void e1000_print_mac_address()
{
    kprintf("[ " KBOLD KGRN "OK" KRESET KWHT " ] ");
    kprintf("Intel e1000 MAC Address: %s\n", get_mac_address_string(mac));
}

void e1000_linkup()
{
    uint32_t val = e1000_read_command(REG_CTRL);
    val |= ECTRL_SLU;
    e1000_write_command(REG_CTRL, val);
}

bool e1000_start()
{
    e1000_detect_eeprom();
    if (!e1000_read_mac_address()) {
        return false;
    }
    e1000_print_mac_address();
    e1000_linkup();

    // Clear multicast table array
    for (int i = 0; i < 0x80; i++)
        e1000_write_command(0x5200 + i * 4, 0);

    if (idt_register_interrupt_callback(IRQ0 + pci_device->header.irq, e1000_interrupt_handler) == 0) {
        e1000_enable_interrupt();
        e1000_rx_init();
        e1000_tx_init();

        dhcp_send_request(mac);
        return true;
    }
    return false;
}

/// @brief Process all available packets in the receive ring
void e1000_receive_packets()
{
    while (true) {
        struct e1000_rx_desc *rx_desc = rx_descs[rx_cur];
        const uint16_t len            = rx_descs[rx_cur]->length;

        // Check if the descriptor is ready (Descriptor Done - DD bit set)
        if (!(rx_desc->status & E1000_RXD_STAT_DD)) {
            break; // No more packets to process
        }

        // Check if the packet has End of Packet (EOP) set
        if (!(rx_desc->status & E1000_RXD_STAT_EOP)) {
            // TODO: Handle the error: packet is not complete
            warningf("Incomplete packet received\n");
            continue;
        }

        network_receive((uint8_t *)rx_desc->addr, len);

        // Clear the status to indicate the descriptor is free
        rx_desc->status = 0;

        // Update the tail index
        rx_cur = (rx_cur + 1) % E1000_RX_RING_SIZE;
    }

    // Update the Receive Descriptor Tail (RDT) register
    e1000_write_command(REG_RXDESCTAIL, rx_cur);
}

// void e1000_receive()
// {
//     while ((rx_descs[rx_cur]->status & 0x1)) {
//         auto const buf     = (uint8_t *)rx_descs[rx_cur]->addr;
//         const uint16_t len = rx_descs[rx_cur]->length;
//
//         network_receive(buf, len);
//
//         rx_descs[rx_cur]->status = 0;
//         const uint16_t old_cur   = rx_cur;
//         rx_cur                   = (rx_cur + 1) % E1000_NUM_RX_DESC;
//         e1000_write_command(REG_RXDESCTAIL, old_cur);
//     }
// }

int e1000_send_packet(const void *data, const uint16_t len)
{
    tx_descs[tx_cur]->addr   = (uint64_t)data;
    tx_descs[tx_cur]->length = len;
    tx_descs[tx_cur]->cmd    = CMD_EOP | CMD_IFCS | CMD_RS;
    tx_descs[tx_cur]->status = 0;
    const uint8_t old_cur    = tx_cur;
    tx_cur                   = (tx_cur + 1) % E1000_TX_RING_SIZE;
    e1000_write_command(REG_TXDESCTAIL, tx_cur);
    while (!(tx_descs[old_cur]->status & 0xff))
        ;

    return 0;
}