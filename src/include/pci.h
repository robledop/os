#pragma once

#include "types.h"

struct pci_t
{
    uint16_t vendorID;
    uint16_t deviceID;
    uint16_t command;
    uint16_t status;
    uint8_t revisionID;
    uint8_t progIF;
    uint8_t subclass;
    uint8_t class;
    uint8_t cacheLineSize;
    uint8_t latencyTimer;
    uint8_t headerType;
    uint8_t BIST;
    uint32_t BAR0;
    uint32_t BAR1;
    uint32_t BAR2;
    uint32_t BAR3;
    uint32_t BAR4;
    uint32_t BAR5;
    uint32_t cardbusCISPointer;
    uint16_t subsystemVendorID;
    uint16_t subsystemID;
    uint32_t expansionROMBaseAddress;
    uint8_t capabilitiesPointer;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t reserved2;
    uint8_t irq;
    uint8_t interruptPIN;
    uint8_t minGrant;
    uint8_t maxLatency;
} __attribute__((packed));

void pci_scan();
