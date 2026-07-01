#include "storage_hw.h"
#include "paging_builder.h"

#define X86_64_PCI_BAR_IO_SPACE 0x00000001U
#define X86_64_PCI_BAR_MEM_MASK 0xFFFFFFF0U

static u32 ahci_bar_is_memory(u32 bar)
{
    return ((bar != 0U) && ((bar & X86_64_PCI_BAR_IO_SPACE) == 0U)) ? 1U : 0U;
}

void x86_64_storage_hw_init(struct x86_64_storage_hw_state *state,
                            const struct x86_64_pci_state *pci_state)
{
    state->initialized = 0U;
    state->pci_ready = 0U;
    state->pci_devices = 0U;
    state->storage_controllers = 0U;
    state->ahci_controllers = 0U;
    state->nvme_controllers = 0U;
    state->ahci_controller_found = 0U;
    state->ahci_bar5_raw = 0U;
    state->ahci_mmio_base = 0ULL;
    state->ahci_mmio_virtual_address = 0ULL;
    state->ahci_mmio_bar_ready = 0U;
    state->ahci_mmio_mapped = 0U;
    state->block_driver_ready = 0U;
    state->persistent_ready = 0U;

    if (pci_state == 0) {
        return;
    }

    state->pci_ready = pci_state->initialized;
    state->pci_devices = pci_state->devices_found;
    state->storage_controllers = pci_state->storage_controllers;
    state->ahci_controllers = pci_state->ahci_controllers;
    state->nvme_controllers = pci_state->nvme_controllers;

    if (pci_state->first_ahci_found != 0U) {
        state->ahci_controller_found = 1U;
        state->ahci_bar5_raw = pci_state->first_ahci.bar5;
        if (ahci_bar_is_memory(state->ahci_bar5_raw) != 0U) {
            state->ahci_mmio_base = (u64)(state->ahci_bar5_raw & X86_64_PCI_BAR_MEM_MASK);
            state->ahci_mmio_bar_ready = 1U;
        }
    }

    state->initialized = 1U;
}

void x86_64_storage_hw_apply_ahci_mapping(struct x86_64_storage_hw_state *state,
                                          const struct x86_64_paging_builder_state *builder)
{
    if ((state == 0) || (builder == 0)) {
        return;
    }

    if ((state->ahci_mmio_bar_ready != 0U) &&
        (builder->ahci_mmio_entry_ok != 0U)) {
        state->ahci_mmio_virtual_address = builder->ahci_mmio_virtual_address;
        state->ahci_mmio_mapped = 1U;
    }
}
