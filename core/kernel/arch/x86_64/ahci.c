#include "ahci.h"
#include "pmm.h"

#define X86_64_AHCI_CAP_S64A (1U << 31U)
#define X86_64_AHCI_CAP_SNCQ (1U << 30U)
#define X86_64_AHCI_CAP_NCS_SHIFT 8U
#define X86_64_AHCI_CAP_NCS_MASK 0x1FU
#define X86_64_AHCI_MAX_PORTS 32U
#define X86_64_AHCI_PORT_DET_PRESENT 0x03U
#define X86_64_AHCI_PORT_IPM_ACTIVE 0x01U
#define X86_64_AHCI_SIG_ATAPI 0xEB140101U
#define X86_64_AHCI_COMMAND_ENGINE_PAGES 4U
#define X86_64_AHCI_PORT_CMD_ST (1U << 0U)
#define X86_64_AHCI_PORT_CMD_FRE (1U << 4U)
#define X86_64_AHCI_PORT_CMD_FR (1U << 14U)
#define X86_64_AHCI_PORT_CMD_CR (1U << 15U)
#define X86_64_AHCI_ENGINE_SPIN_LIMIT 100000U

struct x86_64_ahci_hba_memory {
    volatile u32 cap;
    volatile u32 ghc;
    volatile u32 is;
    volatile u32 pi;
};

struct x86_64_ahci_port_registers {
    volatile u32 clb;
    volatile u32 clbu;
    volatile u32 fb;
    volatile u32 fbu;
    volatile u32 is;
    volatile u32 ie;
    volatile u32 cmd;
    volatile u32 rsv0;
    volatile u32 tfd;
    volatile u32 sig;
    volatile u32 ssts;
};

static void clear_page(u64 page)
{
    u8 *bytes = (u8 *)page;
    for (usize i = 0U; i < X86_64_PAGE_SIZE; ++i) {
        bytes[i] = 0U;
    }
}

static u32 lower32(u64 value)
{
    return (u32)(value & 0xFFFFFFFFULL);
}

static u32 upper32(u64 value)
{
    return (u32)((value >> 32U) & 0xFFFFFFFFULL);
}

static u32 allocate_page(u64 *page)
{
    u64 allocated = x86_64_pmm_alloc_page();
    if (allocated == X86_64_PMM_INVALID_PAGE) {
        *page = 0ULL;
        return 0U;
    }

    clear_page(allocated);
    *page = allocated;
    return 1U;
}

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
    state->ports_scanned = 0U;
    state->device_ports = 0U;
    state->sata_device_ports = 0U;
    state->atapi_device_ports = 0U;
    state->first_device_port = 0U;
    state->first_device_signature = 0U;
    state->first_device_status = 0U;
    state->command_buffers_allocated = 0U;
    state->command_list_page = 0ULL;
    state->received_fis_page = 0ULL;
    state->command_table_page = 0ULL;
    state->dma_buffer_page = 0ULL;
    state->command_port_programmed = 0U;
    state->command_list_bound = 0U;
    state->received_fis_bound = 0U;
    state->command_engine_stopped = 0U;
    state->command_engine_started = 0U;
    state->command_engine_ready = 0U;
    state->driver_ready = 0U;
}

static void wait_for_port_bits_clear(struct x86_64_ahci_port_registers *port_regs,
                                     u32 bits,
                                     u32 *cleared)
{
    *cleared = 0U;
    for (u32 i = 0U; i < X86_64_AHCI_ENGINE_SPIN_LIMIT; ++i) {
        if ((port_regs->cmd & bits) == 0U) {
            *cleared = 1U;
            return;
        }
    }
}

static void allocate_command_engine_buffers(struct x86_64_ahci_state *state)
{
    if (state->sata_device_ports == 0U) {
        return;
    }

    if ((allocate_page(&state->command_list_page) == 0U) ||
        (allocate_page(&state->received_fis_page) == 0U) ||
        (allocate_page(&state->command_table_page) == 0U) ||
        (allocate_page(&state->dma_buffer_page) == 0U)) {
        return;
    }

    state->command_buffers_allocated = X86_64_AHCI_COMMAND_ENGINE_PAGES;
}

static struct x86_64_ahci_port_registers *selected_port_regs(
    const struct x86_64_ahci_state *state,
    const struct x86_64_storage_hw_state *storage)
{
    u64 port_base = storage->ahci_mmio_virtual_address +
                    0x100ULL + ((u64)state->first_device_port * 0x80ULL);
    return (struct x86_64_ahci_port_registers *)port_base;
}

static void bind_command_engine_buffers(struct x86_64_ahci_state *state,
                                        const struct x86_64_storage_hw_state *storage)
{
    struct x86_64_ahci_port_registers *port_regs;
    u32 clb_hi = upper32(state->command_list_page);
    u32 fb_hi = upper32(state->received_fis_page);

    if (state->command_buffers_allocated != X86_64_AHCI_COMMAND_ENGINE_PAGES) {
        return;
    }

    if ((state->hba_64bit_capable == 0U) && ((clb_hi != 0U) || (fb_hi != 0U))) {
        return;
    }

    port_regs = selected_port_regs(state, storage);

    port_regs->clb = lower32(state->command_list_page);
    port_regs->clbu = clb_hi;
    port_regs->fb = lower32(state->received_fis_page);
    port_regs->fbu = fb_hi;

    state->command_list_bound =
        ((port_regs->clb == lower32(state->command_list_page)) &&
         (port_regs->clbu == clb_hi)) ? 1U : 0U;
    state->received_fis_bound =
        ((port_regs->fb == lower32(state->received_fis_page)) &&
         (port_regs->fbu == fb_hi)) ? 1U : 0U;
    state->command_port_programmed =
        ((state->command_list_bound != 0U) &&
         (state->received_fis_bound != 0U)) ? 1U : 0U;
}

static void start_command_engine(struct x86_64_ahci_state *state,
                                 const struct x86_64_storage_hw_state *storage)
{
    struct x86_64_ahci_port_registers *port_regs;
    u32 stopped;

    if (state->command_port_programmed == 0U) {
        return;
    }

    port_regs = selected_port_regs(state, storage);
    port_regs->cmd &= ~(X86_64_AHCI_PORT_CMD_ST | X86_64_AHCI_PORT_CMD_FRE);
    wait_for_port_bits_clear(port_regs,
                             X86_64_AHCI_PORT_CMD_CR | X86_64_AHCI_PORT_CMD_FR,
                             &stopped);
    state->command_engine_stopped = stopped;
    if (state->command_engine_stopped == 0U) {
        return;
    }

    port_regs->cmd |= X86_64_AHCI_PORT_CMD_FRE;
    port_regs->cmd |= X86_64_AHCI_PORT_CMD_ST;
    state->command_engine_started =
        ((port_regs->cmd & (X86_64_AHCI_PORT_CMD_ST | X86_64_AHCI_PORT_CMD_FRE)) ==
         (X86_64_AHCI_PORT_CMD_ST | X86_64_AHCI_PORT_CMD_FRE)) ? 1U : 0U;
    state->command_engine_ready = state->command_engine_started;
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
            (const struct x86_64_ahci_hba_memory *)storage->ahci_mmio_virtual_address;
        u32 cap = hba->cap;
        state->hba_ports_implemented = hba->pi;
        state->hba_command_slots = ((cap >> X86_64_AHCI_CAP_NCS_SHIFT) &
                                    X86_64_AHCI_CAP_NCS_MASK) + 1U;
        state->hba_64bit_capable = ((cap & X86_64_AHCI_CAP_S64A) != 0U) ? 1U : 0U;
        state->hba_ncq_capable = ((cap & X86_64_AHCI_CAP_SNCQ) != 0U) ? 1U : 0U;
        state->hba_registers_read = 1U;

        for (u32 port = 0U; port < X86_64_AHCI_MAX_PORTS; ++port) {
            u32 port_bit = 1U << port;
            if ((state->hba_ports_implemented & port_bit) == 0U) {
                continue;
            }

            const struct x86_64_ahci_port_registers *port_regs =
                (const struct x86_64_ahci_port_registers *)
                (storage->ahci_mmio_virtual_address + 0x100ULL + ((u64)port * 0x80ULL));
            u32 ssts = port_regs->ssts;
            u32 det = ssts & 0x0FU;
            u32 ipm = (ssts >> 8U) & 0x0FU;
            u32 sig = port_regs->sig;

            if ((det != X86_64_AHCI_PORT_DET_PRESENT) ||
                (ipm != X86_64_AHCI_PORT_IPM_ACTIVE)) {
                continue;
            }

            state->device_ports |= port_bit;
            if (state->ports_scanned == 0U) {
                state->first_device_port = port;
                state->first_device_signature = sig;
                state->first_device_status = ssts;
            }

            if (sig == X86_64_AHCI_SIG_ATAPI) {
                state->atapi_device_ports |= port_bit;
            } else {
                state->sata_device_ports |= port_bit;
            }

            state->ports_scanned++;
        }

        allocate_command_engine_buffers(state);
        bind_command_engine_buffers(state, storage);
        start_command_engine(state, storage);
    }

    state->initialized = 1U;
}
