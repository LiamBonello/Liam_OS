#ifndef LIAM_OS_X86_64_STORAGE_HW_H
#define LIAM_OS_X86_64_STORAGE_HW_H

#include "pci.h"
#include "types.h"

struct x86_64_paging_builder_state;

struct x86_64_storage_hw_state {
    u32 initialized;
    u32 pci_ready;
    u32 pci_devices;
    u32 storage_controllers;
    u32 ahci_controllers;
    u32 nvme_controllers;
    u32 ahci_controller_found;
    u32 ahci_bar5_raw;
    u64 ahci_mmio_base;
    u64 ahci_mmio_virtual_address;
    u32 ahci_mmio_bar_ready;
    u32 ahci_mmio_mapped;
    u32 block_driver_ready;
    u32 persistent_ready;
};

void x86_64_storage_hw_init(struct x86_64_storage_hw_state *state,
                            const struct x86_64_pci_state *pci_state);
void x86_64_storage_hw_apply_ahci_mapping(struct x86_64_storage_hw_state *state,
                                          const struct x86_64_paging_builder_state *builder);

#endif
