#ifndef LIAM_OS_X86_64_PCI_H
#define LIAM_OS_X86_64_PCI_H

#include "types.h"

#define X86_64_PCI_VENDOR_NONE 0xFFFFU

struct x86_64_pci_device_ref {
    u8 bus;
    u8 device;
    u8 function;
    u16 vendor_id;
    u16 device_id;
    u8 class_code;
    u8 subclass;
    u8 prog_if;
    u32 bar0;
    u32 bar5;
};

struct x86_64_pci_state {
    u32 initialized;
    u32 config_io_ready;
    u32 devices_found;
    u32 multifunction_devices;
    u32 storage_controllers;
    u32 ahci_controllers;
    u32 nvme_controllers;
    u32 first_ahci_found;
    u32 first_nvme_found;
    struct x86_64_pci_device_ref first_ahci;
    struct x86_64_pci_device_ref first_nvme;
};

void x86_64_pci_scan(struct x86_64_pci_state *state);

#endif
