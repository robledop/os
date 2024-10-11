#include "pci.h"
#include "types.h"
#include "io.h"
#include "terminal.h"

// https://wiki.osdev.org/PCI

struct pci_driver
{
    bool used;
    uint8_t class;
    uint8_t subclass;
    void (*pci_function)(struct pci_t, uint8_t, uint8_t, uint8_t);
};

struct pci_driver pci_drivers[255];

struct pci_class_name
{
    uint8_t class;
    uint8_t subclass;
    const char *name;
};

struct pci_class_name classnames[] = {
    {0x00, 0x00, "Non-VGA-Compatible Unclassified Device"},
    {0x00, 0x01, "VGA-Compatible Unclassified Device"},
    {0x01, 0x00, "SCSI Bus Controller"},
    {0x01, 0x01, "IDE Controller"},
    {0x01, 0x02, "Floppy Disk Controller"},
    {0x01, 0x03, "IPI Bus Controller"},
    {0x01, 0x04, "RAID Controller"},
    {0x01, 0x05, "ATA Controller"},
    {0x01, 0x06, "Serial ATA Controller"},
    {0x01, 0x07, "Serial Attached SCSI Controller"},
    {0x01, 0x08, "Non-Volatile Memory Controller"},
    {0x01, 0x80, "Other Mass Storage Controller"},
    {0x02, 0x00, "Ethernet Controller"},
    {0x02, 0x01, "Token Ring Controller"},
    {0x02, 0x02, "FDDI Controller"},
    {0x02, 0x03, "ATM Controller"},
    {0x02, 0x04, "ISDN Controller"},
    {0x02, 0x05, "WorldFip Controller"},
    {0x02, 0x06, "PICMG 2.14 Multi Computing Controller"},
    {0x02, 0x07, "Infiniband Controller"},
    {0x02, 0x08, "Fabric Controller"},
    {0x02, 0x80, "Other Network Controller"},
    {0x03, 0x00, "VGA Compatible Controller"},
    {0x03, 0x01, "XGA Controller"},
    {0x03, 0x02, "3D Controller (Not VGA-Compatible)"},
    {0x03, 0x80, "Other Display Controller"},
    {0x04, 0x00, "Multimedia Video Controller"},
    {0x04, 0x01, "Multimedia Audio Controller"},
    {0x04, 0x02, "Computer Telephony Device"},
    {0x04, 0x03, "Audio Device"},
    {0x04, 0x80, "Other Multimedia Controller"},
    {0x05, 0x00, "RAM Controller"},
    {0x05, 0x01, "Flash Controller"},
    {0x05, 0x80, "Other Memory Controller"},
    {0x06, 0x00, "Host Bridge"},
    {0x06, 0x01, "ISA Bridge"},
    {0x06, 0x02, "EISA Bridge"},
    {0x06, 0x03, "MCA Bridge"},
    {0x06, 0x04, "PCI-to-PCI Brige"},
    {0x06, 0x05, "PCMCIA Bridge"},
    {0x06, 0x06, "NuBus Bridge"},
    {0x06, 0x07, "CardBus Bridge"},
    {0x06, 0x08, "RACEway Bridge"},
    {0x06, 0x09, "PCI-to-PCI Bridge"},
    {0x06, 0x0A, "Infiniband-to-PCI Host Bridge"},
    {0x06, 0x80, "Other Bridge"},
    {0x07, 0x00, "Serial Controller"},
    {0x07, 0x01, "Parallel Controller"},
    {0x07, 0x02, "Multiport Serial Controller"},
    {0x07, 0x03, "Modem"},
    {0x07, 0x04, "IEEE 488.1/2 (GPIB) Controller"},
    {0x07, 0x05, "Smart Card Controller"},
    {0x07, 0x80, "Other Simple Communication Controller"},
    {0x08, 0x00, "PIC"},
    {0x08, 0x01, "DMA Controller"},
    {0x08, 0x02, "Timer"},
    {0x08, 0x03, "RTC Controller"},
    {0x08, 0x04, "PCI Hot-Plug Controller"},
    {0x08, 0x05, "SD Host Controller"},
    {0x08, 0x07, "IOMMU"},
    {0x08, 0x80, "Other Base System Peripheral"},
    {0x09, 0x00, "Keyboard Controller"},
    {0x09, 0x01, "Digitizer Pen"},
    {0x09, 0x02, "Mouse Controller"},
    {0x09, 0x03, "Scanner Controller"},
    {0x09, 0x04, "Gameport Controller"},
    {0x09, 0x80, "Other Input Device Controller"},
    {0x0A, 0x00, "Generic Docking Station"},
    {0x0A, 0x80, "Other Docking Station"},
    {0x0B, 0x00, "386 Processor"},
    {0x0B, 0x01, "486 Processor"},
    {0x0B, 0x02, "Pentium Processor"},
    {0x0B, 0x03, "Pentioum Pro Processor"},
    {0x0B, 0x10, "Alpha Processor"},
    {0x0B, 0x20, "PowerPC Processor"},
    {0x0B, 0x30, "MIPS Processor"},
    {0x0B, 0x40, "Co-Processor"},
    {0x0B, 0x80, "Other Processor"},
    {0x0C, 0x00, "FireWire (IEEE 1394) Controller"},
    {0x0C, 0x01, "ACCESS Bus Controller"},
    {0x0C, 0x02, "SSA"},
    {0x0C, 0x03, "USB Controller"},
    {0x0C, 0x04, "Fibre Channel"},
    {0x0C, 0x05, "SMBus Controller"},
    {0x0C, 0x06, "InfiniBand Controller"},
    {0x0C, 0x07, "IPMI Interface"},
    {0x0C, 0x08, "SERCOS Interface (IEC 61491)"},
    {0x0C, 0x09, "CANbus Controller"},
    {0x0C, 0x80, "Other Serial Bus Controller"},
    {0x0D, 0x00, "iRDA Compatible Controller"},
    {0x0D, 0x00, "Consumer IR Controller"},
    {0x0D, 0x00, "RF Controller"},
    {0x0D, 0x00, "Bluetooth Controller"},
    {0x0D, 0x00, "Broadband Controller"},
    {0x0D, 0x00, "Ethernet Controller (802.1a)"},
    {0x0D, 0x00, "Ethernet Controller (802.1b)"},
    {0x0D, 0x00, "Other Wireless Controller"},
    {0x0E, 0x00, "I20"},
    {0x0F, 0x01, "Satellite TV Controller"},
    {0x0F, 0x02, "Satellite Audio Controller"},
    {0x0F, 0x03, "Satellite Voice Controller"},
    {0x0F, 0x04, "Satellite Data Controller"},
    {0x10, 0x00, "Network and Computing Encryption/Decryption"},
    {0x10, 0x10, "Entertainment Encryption/Decryption"},
    {0x10, 0x80, "Other Encryption Controller"},
    {0x11, 0x00, "DPIO Modules"},
    {0x11, 0x01, "Performance Counters"},
    {0x11, 0x10, "Communication Synchronizer"},
    {0x11, 0x20, "Signal Processing Management"},
    {0x11, 0x80, "Other Signal Processing Controller"},

    {0x12, 0x00, "Processing Accelerator"},
    {0x13, 0x00, "Non-Essential Instrumentation"},
    {0x40, 0x00, "Co-Processor"},
    {0xFF, 0x00, "Vendor Specific"},
};

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;

    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                         (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    // Write out the address
    outl(0xCF8, address);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

const char *pci_find_name(uint8_t class, uint8_t subclass)
{
    for (size_t i = 0; i < sizeof(classnames) / sizeof(struct pci_class_name); i++)
    {
        if (classnames[i].class == class && classnames[i].subclass == subclass)
        {
            return classnames[i].name;
        }
    }
    return "Unknown PCI Device";
}

void load_driver(struct pci_t pci, uint8_t bus, uint8_t device, uint8_t function)
{
    for (uint16_t i = 0; i < 255; i++)
    {
        if (pci_drivers[i].used && pci_drivers[i].class == pci.class && pci_drivers[i].subclass == pci.subclass)
        {
            kprintf("%x.%x.%d: Loading the driver for %s\n", bus, device, function, pci_find_name(pci.class, pci.subclass));
            pci_drivers[i].pci_function(pci, bus, device, function);
            return;
        }
    }
    kprintf("%x.%x.%d: No driver for %s\n", bus, device, function, pci_find_name(pci.class, pci.subclass));
}

// uint16_t pci_check_vendor(uint8_t bus, uint8_t slot)
// {
//     uint16_t vendor, device;
//     /* Try and read the first configuration register. Since there are no
//      * vendors that == 0xFFFF, it must be a non-existent device. */
//     if ((vendor = pci_config_read_Word(bus, slot, 0, 0)) != 0xFFFF)
//     {
//         device = pci_config_read_Word(bus, slot, 0, 2);
//         //    . . .
//     }
//     return (vendor);
// }

// void pci_check_function(uint8_t bus, uint8_t device, uint8_t function)
// {
//     uint8_t baseClass;
//     uint8_t subClass;
//     uint8_t secondaryBus;

//     baseClass = getBaseClass(bus, device, function);
//     subClass = getSubClass(bus, device, function);
//     if ((baseClass == 0x6) && (subClass == 0x4))
//     {
//         secondaryBus = getSecondaryBus(bus, device, function);
//         checkBus(secondaryBus);
//     }
// }

struct pci_t get_pci_data(uint8_t bus, uint8_t num, uint8_t function)
{
    struct pci_t pci_data;
    uint16_t *p = (uint16_t *)&pci_data;
    for (uint8_t i = 0; i < 32; i++)
    {
        p[i] = pci_config_read_word(bus, num, function, i * 2);
    }
    return pci_data;
}

// void pci_check_device(uint8_t bus, uint8_t device)
// {
//     uint8_t function = 0;

//     vendorID = getVendorID(bus, device, function);
//     if (vendorID == 0xFFFF)
//         return; // Device doesn't exist
//     pci_check_function(bus, device, function);
//     headerType = getHeaderType(bus, device, function);
//     if ((headerType & 0x80) != 0)
//     {
//         // It's a multi-function device, so check remaining functions
//         for (function = 1; function < 8; function++)
//         {
//             if (getVendorID(bus, device, function) != 0xFFFF)
//             {
//                 checkFunction(bus, device, function);
//             }
//         }
//     }
// }

// void pci_check_all_buses(void)
// {
//     uint16_t bus;
//     uint8_t device;

//     for (bus = 0; bus < 256; bus++)
//     {
//         for (device = 0; device < 32; device++)
//         {
//             pci_check_device(bus, device);
//         }
//     }
// }

void pci_scan()
{
    kprintf("Scanning PCI devices...\n");

    struct pci_t c_pci;
    for (uint16_t i = 0; i < 256; i++)
    {
        c_pci = get_pci_data(i, 0, 0);
        if (c_pci.vendorID != 0xFFFF)
        {
            for (uint8_t j = 0; j < 32; j++)
            {
                c_pci = get_pci_data(i, j, 0);
                if (c_pci.vendorID != 0xFFFF)
                {
                    load_driver(c_pci, i, j, 0);

                    for (uint8_t k = 1; k < 8; k++)
                    {
                        struct pci_t pci = get_pci_data(i, j, k);
                        if (pci.vendorID != 0xFFFF)
                        {
                            load_driver(pci, i, j, k);
                        }
                    }
                }
            }
        }
    }
}