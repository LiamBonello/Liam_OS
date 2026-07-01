#ifndef LIAM_OS_X86_64_STORAGE_HW_H
#define LIAM_OS_X86_64_STORAGE_HW_H

#include "pci.h"
#include "types.h"

struct x86_64_storage_hw_state {
    u32 initialized;
    u32 pci_ready;
    u32 pci_devices;
    u32 storage_controllers;
    u32 ahci_controllers;
    u32 nvme_controllers;
    u32 block_driver_ready;
    u32 persistent_ready;
};

void x86_64_storage_hw_init(struct x86_64_storage_hw_state *state,
                            const struct x86_64_pci_state *pci_state);

#endif
