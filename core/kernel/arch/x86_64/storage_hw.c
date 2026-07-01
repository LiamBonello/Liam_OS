#include "storage_hw.h"

void x86_64_storage_hw_init(struct x86_64_storage_hw_state *state,
                            const struct x86_64_pci_state *pci_state)
{
    state->initialized = 0U;
    state->pci_ready = 0U;
    state->pci_devices = 0U;
    state->storage_controllers = 0U;
    state->ahci_controllers = 0U;
    state->nvme_controllers = 0U;
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

    state->initialized = 1U;
}
