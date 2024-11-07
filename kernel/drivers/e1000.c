#include <e1000.h>
#include <idt.h>
#include <io.h>
#include <kernel_heap.h>
#include <net/ethernet.h>
#include <string.h>
#include <vga_buffer.h>

#define IRQ0 0x20

void e1000_handle_receive();
bool e1000_start();
void e1000_linkup();
int e1000_send_packet(const void *p_data, uint16_t p_len);

static uint8_t bar_type;                                  // Type of BAR0
static uint16_t io_base;                                  // IO Base Address
static uint64_t mem_base;                                 // MMIO Base Address
static bool eeprom_exists;                                // A flag indicating if eeprom exists
static uint8_t mac[6];                                    // A buffer for storing the mack address
static struct e1000_rx_desc *rx_descs[E1000_NUM_RX_DESC]; // Receive Descriptor Buffers
static struct e1000_tx_desc *tx_descs[E1000_NUM_TX_DESC]; // Transmit Descriptor Buffers
static uint16_t rx_cur;                                   // Current Receive Descriptor Buffer
static uint16_t tx_cur;                                   // Current Transmit Descriptor Buffer
static struct pci_device *pci_device;

static uint8_t read8(const uint64_t addr)
{
    return *(volatile uint8_t *)addr;
}

static uint16_t read16(const uint64_t addr)
{
    return *(volatile uint16_t *)addr;
}

static uint32_t read32(const uint64_t addr)
{
    return *(volatile uint32_t *)addr;
}

static uint64_t read64(const uint64_t addr)
{
    return *(volatile uint64_t *)addr;
}

static void write8(const uint64_t addr, const uint8_t data)
{
    *(volatile uint8_t *)addr = data;
}

static void write16(const uint64_t addr, const uint16_t data)
{
    *(volatile uint16_t *)addr = data;
}

static void write32(const uint64_t addr, const uint32_t data)
{
    *(volatile uint64_t *)addr = data;
}

static void write64(const uint64_t addr, const uint64_t data)
{
    *(volatile uint64_t *)addr = data;
}

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
    return true;
}

void e1000_rx_init()
{
    const uint8_t *ptr                = (uint8_t *)(kmalloc(sizeof(struct e1000_rx_desc) * E1000_NUM_RX_DESC + 16));
    const struct e1000_rx_desc *descs = (struct e1000_rx_desc *)ptr;

    for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
        rx_descs[i]         = (struct e1000_rx_desc *)((uint8_t *)descs + i * 16);
        rx_descs[i]->addr   = (uint64_t)(uint8_t *)(kmalloc(8192 + 16));
        rx_descs[i]->status = 0;
    }

    e1000_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32));
    e1000_write_command(REG_TXDESCHI, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

    e1000_write_command(REG_RXDESCLO, (uint64_t)ptr);
    e1000_write_command(REG_RXDESCHI, 0);

    e1000_write_command(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

    e1000_write_command(REG_RXDESCHEAD, 0);
    e1000_write_command(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
    rx_cur = 0;
    e1000_write_command(REG_RCTRL,
                        RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC |
                            RCTL_BSIZE_8192);
}

void e1000_tx_init()
{
    const uint8_t *ptr                = (uint8_t *)(kmalloc(sizeof(struct e1000_tx_desc) * E1000_NUM_TX_DESC + 16));
    const struct e1000_tx_desc *descs = (struct e1000_tx_desc *)ptr;

    for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
        tx_descs[i]         = (struct e1000_tx_desc *)((uint8_t *)descs + i * 16);
        tx_descs[i]->addr   = 0;
        tx_descs[i]->cmd    = 0;
        tx_descs[i]->status = TSTA_DD;
    }

    e1000_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32));
    e1000_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

    // now setup total length of descriptors
    e1000_write_command(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);

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
        kprintf("E1000 started\n");
    }
}

void e1000_fire(int interrupt, const struct interrupt_frame *frame)
{
    if (interrupt == pci_device->header.irq + IRQ0) {
        // /* This might be needed here if your handler doesn't clear interrupts from each device and must be done
        // before
        //    EOI if using the PIC. Without this, the card will spam interrupts as the int-line will stay high. */
        e1000_write_command(REG_IMASK, 0x1);

        const uint32_t status = e1000_read_command(0xc0);
        if (status & 0x04) {
            e1000_linkup();
        } else if (status & 0x10) {
            // good threshold
        } else if (status & 0x80) {
            e1000_handle_receive();
        }
    }
}

void e1000_print_mac_address()
{
    kprintf("Intel e1000 MAC Address: ");
    for (int i = 0; i < 6; i++) {
        char str[3];
        itohex(mac[i], str);
        kprintf(str);
        if (i < 5) {
            kprintf(":");
        }
    }
    kprintf("\n");
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

    if (idt_register_interrupt_callback(IRQ0 + pci_device->header.irq, e1000_fire) == 0) {
        e1000_enable_interrupt();
        e1000_rx_init();
        e1000_tx_init();
        return true;
    } else {
        return false;
    }
}

uint16_t ntohs(uint16_t data)
{
    return ((data & 0x00ff) << 8) | (data & 0xff00) >> 8;
}

void e1000_handle_receive()
{
    uint16_t old_cur;
    bool got_packet = false;

    while ((rx_descs[rx_cur]->status & 0x1)) {
        got_packet   = true;
        uint8_t *buf = (uint8_t *)rx_descs[rx_cur]->addr;
        uint16_t len = rx_descs[rx_cur]->length;

        struct ether_header *hdr = (struct ether_header *)buf;
        kprintf(KBOLD KBLU "Source MAC: " KRESET KWHT);

        for (int i = 0; i < 6; i++) {
            char str[3];
            itohex(hdr->src_host[i], str);
            kprintf(str);
            if (i < 5) {
                kprintf(":");
            }
        }
        kprintf("\n");

        kprintf(KBOLD KBLU "Destination MAC: " KRESET KWHT);
        for (int i = 0; i < 6; i++) {
            char str[3];
            itohex(hdr->dest_host[i], str);
            kprintf(str);
            if (i < 5) {
                kprintf(":");
            }
        }
        kprintf("\n");

        kprintf(KBOLD KBLU "EtherType: " KRESET KWHT "%x\n", ntohs(hdr->ether_type));

        for (int i = sizeof(struct ether_header); i < len; i++) {
            if (buf[i]) {
                kprintf("%c", buf[i]);
            }
        }

        kprintf("\n");

        rx_descs[rx_cur]->status = 0;
        old_cur                  = rx_cur;
        rx_cur                   = (rx_cur + 1) % E1000_NUM_RX_DESC;
        e1000_write_command(REG_RXDESCTAIL, old_cur);
    }
}

int e1000_send_packet(const void *p_data, uint16_t p_len)
{
    tx_descs[tx_cur]->addr   = (uint64_t)p_data;
    tx_descs[tx_cur]->length = p_len;
    tx_descs[tx_cur]->cmd    = CMD_EOP | CMD_IFCS | CMD_RS;
    tx_descs[tx_cur]->status = 0;
    uint8_t old_cur          = tx_cur;
    tx_cur                   = (tx_cur + 1) % E1000_NUM_TX_DESC;
    e1000_write_command(REG_TXDESCTAIL, tx_cur);
    while (!(tx_descs[old_cur]->status & 0xff))
        ;
    return 0;
}