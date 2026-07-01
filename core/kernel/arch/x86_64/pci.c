#include "pci.h"

#define X86_64_PCI_CONFIG_ADDRESS 0x0CF8U
#define X86_64_PCI_CONFIG_DATA 0x0CFCU
#define X86_64_PCI_ENABLE 0x80000000U
#define X86_64_PCI_MAX_BUS 256U
#define X86_64_PCI_MAX_DEVICE 32U
#define X86_64_PCI_MAX_FUNCTION 8U
#define X86_64_PCI_CLASS_STORAGE 0x01U
#define X86_64_PCI_SUBCLASS_SATA 0x06U
#define X86_64_PCI_PROGIF_AHCI 0x01U
#define X86_64_PCI_SUBCLASS_NVM 0x08U

static inline void outl(u16 port, u32 value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline u32 inl(u16 port)
{
    u32 value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static u32 pci_config_address(u32 bus, u32 device, u32 function, u32 offset)
{
    return X86_64_PCI_ENABLE |
           ((bus & 0xFFU) << 16U) |
           ((device & 0x1FU) << 11U) |
           ((function & 0x07U) << 8U) |
           (offset & 0xFCU);
}

static u32 pci_read32(u32 bus, u32 device, u32 function, u32 offset)
{
    outl((u16)X86_64_PCI_CONFIG_ADDRESS,
         pci_config_address(bus, device, function, offset));
    return inl((u16)X86_64_PCI_CONFIG_DATA);
}

static void clear_device_ref(struct x86_64_pci_device_ref *device)
{
    device->bus = 0U;
    device->device = 0U;
    device->function = 0U;
    device->vendor_id = 0U;
    device->device_id = 0U;
    device->class_code = 0U;
    device->subclass = 0U;
    device->prog_if = 0U;
    device->bar0 = 0U;
    device->bar5 = 0U;
}

static void store_device_ref(struct x86_64_pci_device_ref *device,
                             u32 bus,
                             u32 slot,
                             u32 function,
                             u32 id,
                             u32 class_reg,
                             u32 bar0,
                             u32 bar5)
{
    device->bus = (u8)bus;
    device->device = (u8)slot;
    device->function = (u8)function;
    device->vendor_id = (u16)(id & 0xFFFFU);
    device->device_id = (u16)((id >> 16U) & 0xFFFFU);
    device->class_code = (u8)((class_reg >> 24U) & 0xFFU);
    device->subclass = (u8)((class_reg >> 16U) & 0xFFU);
    device->prog_if = (u8)((class_reg >> 8U) & 0xFFU);
    device->bar0 = bar0;
    device->bar5 = bar5;
}

static void inspect_function(struct x86_64_pci_state *state,
                             u32 bus,
                             u32 device,
                             u32 function)
{
    u32 id = pci_read32(bus, device, function, 0x00U);
    u32 vendor_id = id & 0xFFFFU;
    u32 class_reg;
    u32 class_code;
    u32 subclass;
    u32 prog_if;
    u32 bar0;
    u32 bar5;

    if (vendor_id == X86_64_PCI_VENDOR_NONE) {
        return;
    }

    class_reg = pci_read32(bus, device, function, 0x08U);
    class_code = (class_reg >> 24U) & 0xFFU;
    subclass = (class_reg >> 16U) & 0xFFU;
    prog_if = (class_reg >> 8U) & 0xFFU;
    bar0 = pci_read32(bus, device, function, 0x10U);
    bar5 = pci_read32(bus, device, function, 0x24U);

    state->devices_found++;

    if (class_code != X86_64_PCI_CLASS_STORAGE) {
        return;
    }

    state->storage_controllers++;

    if ((subclass == X86_64_PCI_SUBCLASS_SATA) &&
        (prog_if == X86_64_PCI_PROGIF_AHCI)) {
        state->ahci_controllers++;
        if (state->first_ahci_found == 0U) {
            store_device_ref(&state->first_ahci,
                             bus,
                             device,
                             function,
                             id,
                             class_reg,
                             bar0,
                             bar5);
            state->first_ahci_found = 1U;
        }
    }

    if (subclass == X86_64_PCI_SUBCLASS_NVM) {
        state->nvme_controllers++;
        if (state->first_nvme_found == 0U) {
            store_device_ref(&state->first_nvme,
                             bus,
                             device,
                             function,
                             id,
                             class_reg,
                             bar0,
                             bar5);
            state->first_nvme_found = 1U;
        }
    }
}

void x86_64_pci_scan(struct x86_64_pci_state *state)
{
    u32 bus;
    u32 device;

    state->initialized = 0U;
    state->config_io_ready = 0U;
    state->devices_found = 0U;
    state->multifunction_devices = 0U;
    state->storage_controllers = 0U;
    state->ahci_controllers = 0U;
    state->nvme_controllers = 0U;
    state->first_ahci_found = 0U;
    state->first_nvme_found = 0U;
    clear_device_ref(&state->first_ahci);
    clear_device_ref(&state->first_nvme);

    state->config_io_ready = 1U;

    for (bus = 0U; bus < X86_64_PCI_MAX_BUS; bus++) {
        for (device = 0U; device < X86_64_PCI_MAX_DEVICE; device++) {
            u32 id = pci_read32(bus, device, 0U, 0x00U);
            u32 header;
            u32 function_limit;
            u32 function;

            if ((id & 0xFFFFU) == X86_64_PCI_VENDOR_NONE) {
                continue;
            }

            header = pci_read32(bus, device, 0U, 0x0CU);
            function_limit = (((header >> 16U) & 0x80U) != 0U) ?
                X86_64_PCI_MAX_FUNCTION : 1U;
            if (function_limit == X86_64_PCI_MAX_FUNCTION) {
                state->multifunction_devices++;
            }

            for (function = 0U; function < function_limit; function++) {
                inspect_function(state, bus, device, function);
            }
        }
    }

    state->initialized = 1U;
}
