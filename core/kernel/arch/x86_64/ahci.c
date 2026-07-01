#include "ahci.h"

#define X86_64_AHCI_CAP_S64A (1U << 31U)
#define X86_64_AHCI_CAP_SNCQ (1U << 30U)
#define X86_64_AHCI_CAP_NCS_SHIFT 8U
#define X86_64_AHCI_CAP_NCS_MASK 0x1FU

struct x86_64_ahci_hba_memory {
    volatile u32 cap;
    volatile u32 ghc;
    volatile u32 is;
    volatile u32 pi;
};

static void clear_ahci_state(struct x86_64_ahci_state *state)
{
    state->initialized = 0U;
    state->controller_found = 0U;
    state->bar_ready = 0U;
    state->mmio_mapped = 0U;
    state->hba_probe_safe = 0U;
    state->hba_registers_read = 0U;
    state->hba_ports_implemented = 0U;
    state->hba_command_slots = 0U;
    state->hba_64bit_capable = 0U;
    state->hba_ncq_capable = 0U;
    state->driver_ready = 0U;
}

void x86_64_ahci_probe(struct x86_64_ahci_state *state,
                       const struct x86_64_storage_hw_state *storage)
{
    clear_ahci_state(state);

    if (storage == 0) {
        return;
    }

    state->controller_found = storage->ahci_controller_found;
    state->bar_ready = storage->ahci_mmio_bar_ready;
    state->mmio_mapped = storage->ahci_mmio_mapped;
    state->hba_probe_safe =
        ((state->controller_found != 0U) &&
         (state->bar_ready != 0U) &&
         (state->mmio_mapped != 0U)) ? 1U : 0U;

    if (state->hba_probe_safe != 0U) {
        const struct x86_64_ahci_hba_memory *hba =
            (const struct x86_64_ahci_hba_memory *)storage->ahci_mmio_base;
        u32 cap = hba->cap;
        state->hba_ports_implemented = hba->pi;
        state->hba_command_slots = ((cap >> X86_64_AHCI_CAP_NCS_SHIFT) &
                                    X86_64_AHCI_CAP_NCS_MASK) + 1U;
        state->hba_64bit_capable = ((cap & X86_64_AHCI_CAP_S64A) != 0U) ? 1U : 0U;
        state->hba_ncq_capable = ((cap & X86_64_AHCI_CAP_SNCQ) != 0U) ? 1U : 0U;
        state->hba_registers_read = 1U;
    }

    state->initialized = 1U;
}
