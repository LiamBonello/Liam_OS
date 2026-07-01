#ifndef LIAM_OS_X86_64_AHCI_H
#define LIAM_OS_X86_64_AHCI_H

#include "storage_hw.h"
#include "types.h"

struct x86_64_ahci_state {
    u32 initialized;
    u32 controller_found;
    u32 bar_ready;
    u32 mmio_mapped;
    u32 hba_probe_safe;
    u32 hba_registers_read;
    u32 hba_ports_implemented;
    u32 hba_command_slots;
    u32 hba_64bit_capable;
    u32 hba_ncq_capable;
    u32 ports_scanned;
    u32 device_ports;
    u32 sata_device_ports;
    u32 atapi_device_ports;
    u32 first_device_port;
    u32 first_device_signature;
    u32 first_device_status;
    u32 driver_ready;
};

void x86_64_ahci_probe(struct x86_64_ahci_state *state,
                       const struct x86_64_storage_hw_state *storage);

#endif
