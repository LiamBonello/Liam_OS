BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso
USERLAND_BUILD_DIR := $(BUILD_DIR)/userland
KERNEL := $(BUILD_DIR)/kernel.bin
ISO := $(BUILD_DIR)/liam_os.iso
X86_64_BUILD_DIR := $(BUILD_DIR)/x86_64
X86_64_ISO_DIR := $(X86_64_BUILD_DIR)/iso
X86_64_KERNEL := $(X86_64_BUILD_DIR)/kernel.elf
X86_64_ISO := $(X86_64_BUILD_DIR)/liam_os_x86_64.iso
X86_64_SMOKE_LOG := $(X86_64_BUILD_DIR)/x86_64-smoke.log
X86_64_GRUB_CFG := boot/grub-x86_64.cfg
X86_64_BOOT32_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/boot32.o
X86_64_START_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/start.o
X86_64_IDT_STUBS_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/idt_stubs.o
X86_64_KERNEL_C_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/kernel.o
X86_64_CONSOLE_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/console.o
X86_64_BOOT_INFO_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/boot_info.o
X86_64_BOOT_CONTEXT_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/boot_context.o
X86_64_CPUID_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/cpuid.o
X86_64_GDT_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/gdt.o
X86_64_TSS_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/tss.o
X86_64_MEMORY_LAYOUT_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/memory_layout.o
X86_64_PAGING_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/paging.o
X86_64_PAGING_PLAN_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/paging_plan.o
X86_64_PAGING_BUILDER_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/paging_builder.o
X86_64_PMM_PLAN_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/pmm_plan.o
X86_64_PMM_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/pmm.o
X86_64_IDT_OBJ := $(X86_64_BUILD_DIR)/kernel/arch/x86_64/idt.o
X86_64_BOOT_OBJ := $(X86_64_BOOT32_OBJ) $(X86_64_START_OBJ) $(X86_64_IDT_STUBS_OBJ) $(X86_64_KERNEL_C_OBJ) $(X86_64_CONSOLE_OBJ) $(X86_64_BOOT_INFO_OBJ) $(X86_64_BOOT_CONTEXT_OBJ) $(X86_64_CPUID_OBJ) $(X86_64_GDT_OBJ) $(X86_64_TSS_OBJ) $(X86_64_MEMORY_LAYOUT_OBJ) $(X86_64_PAGING_OBJ) $(X86_64_PAGING_PLAN_OBJ) $(X86_64_PAGING_BUILDER_OBJ) $(X86_64_PMM_PLAN_OBJ) $(X86_64_PMM_OBJ) $(X86_64_IDT_OBJ)

ARCH ?= i386
SUPPORTED_ARCHES := i386 x86_64
ARCH_DIR := kernel/arch/$(ARCH)

ASM := nasm
CC := clang
LD := ld.lld
GRUB_FILE := grub-file
QEMU := qemu-system-i386

ASMFLAGS_i386 := -f elf32
CFLAGS_i386 := -target i386-elf -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -mno-sse -mno-mmx -Wall -Wextra -O0
LDFLAGS_i386 := -T linker.ld -nostdlib -m elf_i386
QEMU_i386 := qemu-system-i386

ASMFLAGS_x86_64 := -f elf64
CFLAGS_x86_64 := -target x86_64-elf -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -mno-red-zone -Wall -Wextra -O0
LDFLAGS_x86_64 := -T linker.x86_64.ld -nostdlib -m elf_x86_64
QEMU_x86_64 := qemu-system-x86_64

ASMFLAGS := $(ASMFLAGS_$(ARCH))
BASE_CFLAGS := $(CFLAGS_$(ARCH))
CFLAGS := $(BASE_CFLAGS)
LDFLAGS := $(LDFLAGS_$(ARCH))
QEMU := $(QEMU_$(ARCH))

ifneq ($(filter $(ARCH),$(SUPPORTED_ARCHES)),$(ARCH))
$(error Unsupported ARCH '$(ARCH)'. Supported: $(SUPPORTED_ARCHES))
endif

ifeq ($(ARCH),x86_64)
$(error ARCH=x86_64 is scaffolded but not the default build path yet. Use 'make x86_64-info', 'make x86_64-kernel', 'make x86_64-iso', 'make x86_64-run', or 'make x86_64-smoke' for staged migration work.)
endif

ASM_SRC := \
	$(ARCH_DIR)/start.asm \
	$(ARCH_DIR)/gdt_load.asm \
	$(ARCH_DIR)/idt_stubs.asm \
	$(ARCH_DIR)/paging_load.asm \
	$(ARCH_DIR)/context_switch.asm \
	$(ARCH_DIR)/irq.asm \
	$(ARCH_DIR)/ring3_entry.asm \
	kernel/userland/userland_images.asm

C_SRC := \
	kernel/core/kernel.c \
	kernel/core/boot.c \
	kernel/core/kernel_symbols.c \
	kernel/core/log.c \
	kernel/core/panic.c \
	kernel/core/assert.c \
	kernel/core/status.c \
	kernel/core/string.c \
	kernel/core/system.c \
	kernel/core/shell.c \
	kernel/core/scheduler.c \
	kernel/core/process.c \
	kernel/core/fd_table.c \
	kernel/core/user_image.c \
	kernel/core/exec.c \
	kernel/core/syscall.c \
	kernel/fs/vfs.c \
	kernel/fs/initramfs.c \
	kernel/loader/flat_binary.c \
	kernel/loader/elf32.c \
	$(ARCH_DIR)/gdt.c \
	$(ARCH_DIR)/tss.c \
	$(ARCH_DIR)/ring3.c \
	$(ARCH_DIR)/idt.c \
	$(ARCH_DIR)/pic.c \
	$(ARCH_DIR)/paging.c \
	kernel/drivers/vga.c \
	kernel/drivers/timer.c \
	kernel/drivers/keyboard.c \
	kernel/memory/pmm.c \
	kernel/memory/heap.c \
	kernel/memory/vmm.c

ASM_OBJ := $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SRC))
C_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRC))

USERLAND_FLAT_NAMES := \
	init \
	hello \
	about \
	pid \
	ticks \
	syscheck \
	sysbadptr \
	os-release \
	cat \
	clear \
	help \
	uptime \
	args \
	echo \
	sh

USERLAND_FLAT_SRC := \
	$(foreach name,$(USERLAND_FLAT_NAMES),userland/flat/$(name)/$(name).asm)

USERLAND_FLAT_BIN := \
	$(addprefix $(USERLAND_BUILD_DIR)/,$(addsuffix .bin,$(USERLAND_FLAT_NAMES)))

VPATH := \
	$(ARCH_DIR) \
	kernel/core \
	kernel/fs \
	kernel/loader \
	kernel/drivers \
	kernel/memory \
	kernel/userland

.PHONY: all clean run run-debug debug check-tools userland x86_64-info x86_64-check-tools x86_64-iso-check-tools x86_64-run-check-tools x86_64-smoke-check-tools x86_64-kernel x86_64-iso x86_64-run x86_64-smoke

all: $(ISO)

debug: CFLAGS := $(BASE_CFLAGS) -DLIAM_OS_DEBUG
debug: clean $(ISO)

check-tools:
	@command -v $(ASM) >/dev/null || (echo "Missing tool: $(ASM)" && exit 1)
	@command -v $(CC) >/dev/null || (echo "Missing tool: $(CC)" && exit 1)
	@command -v $(LD) >/dev/null || (echo "Missing tool: $(LD)" && exit 1)
	@command -v grub-mkrescue >/dev/null || (echo "Missing tool: grub-mkrescue" && exit 1)
	@command -v $(QEMU) >/dev/null || (echo "Missing tool: $(QEMU)" && exit 1)

x86_64-check-tools:
	@command -v $(ASM) >/dev/null || (echo "Missing tool: $(ASM)" && exit 1)
	@command -v $(CC) >/dev/null || (echo "Missing tool: $(CC)" && exit 1)
	@command -v $(LD) >/dev/null || (echo "Missing tool: $(LD)" && exit 1)
	@command -v $(GRUB_FILE) >/dev/null || (echo "Missing tool: $(GRUB_FILE)" && exit 1)

x86_64-iso-check-tools: x86_64-check-tools
	@command -v grub-mkrescue >/dev/null || (echo "Missing tool: grub-mkrescue" && exit 1)

x86_64-run-check-tools: x86_64-iso-check-tools
	@command -v $(QEMU_x86_64) >/dev/null || (echo "Missing tool: $(QEMU_x86_64)" && exit 1)

x86_64-smoke-check-tools: x86_64-run-check-tools

x86_64-info:
	@echo "Liam_OS x86_64 path now runs on C-owned page tables and validates identity, direct-map, and higher-half kernel aliases."
	@echo "Current stable bootable default remains: ARCH=i386"
	@echo "Artifact target: make x86_64-kernel"
	@echo "ISO target: make x86_64-iso"
	@echo "Run target: make x86_64-run"
	@echo "Headless smoke target: make x86_64-smoke"
	@echo "Artifact output: $(X86_64_KERNEL)"
	@echo "ISO output: $(X86_64_ISO)"
	@echo "Smoke log output: $(X86_64_SMOKE_LOG)"
	@echo "Expected x86_64 screen messages: CPU baseline ok: 1, IDT IST gates: 1, PMM smoke free: 1, Desc/IST ok: 1"
	@echo "Expected serial markers include: Paging activation ok: 1, Paging probes ok: 1, VM plan ok: 1, Desc/IST ok: 1"
	@echo "Serial output is routed to the terminal by make x86_64-run and validated by make x86_64-smoke."
	@echo "See: kernel/arch/x86_64/README.md and ../docs/core/x86_64-migration-plan.md"

x86_64-kernel: x86_64-check-tools $(X86_64_KERNEL)
	@echo "Built experimental x86_64 kernel artifact: $(X86_64_KERNEL)"

x86_64-iso: x86_64-iso-check-tools $(X86_64_ISO)
	@echo "Built experimental x86_64 ISO: $(X86_64_ISO)"

x86_64-run: x86_64-run-check-tools $(X86_64_ISO)
	$(QEMU_x86_64) -serial stdio -boot d -cdrom $(X86_64_ISO)

x86_64-smoke: x86_64-smoke-check-tools $(X86_64_ISO)
	sh scripts/x86_64_smoke.sh $(X86_64_SMOKE_LOG)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(X86_64_BUILD_DIR): | $(BUILD_DIR)
	mkdir -p $(X86_64_BUILD_DIR)

$(USERLAND_BUILD_DIR): | $(BUILD_DIR)
	mkdir -p $(USERLAND_BUILD_DIR)

userland: $(USERLAND_FLAT_BIN)

define BUILD_USERLAND_FLAT
$(USERLAND_BUILD_DIR)/$(1).bin: userland/flat/$(1)/$(1).asm | $(USERLAND_BUILD_DIR)
	$$(ASM) -f bin $$< -o $$@
endef

$(foreach name,$(USERLAND_FLAT_NAMES),$(eval $(call BUILD_USERLAND_FLAT,$(name))))

$(X86_64_BOOT32_OBJ): kernel/arch/x86_64/boot32.asm | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS_x86_64) $< -o $@

$(X86_64_START_OBJ): kernel/arch/x86_64/start.asm | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS_x86_64) $< -o $@

$(X86_64_IDT_STUBS_OBJ): kernel/arch/x86_64/idt_stubs.asm | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS_x86_64) $< -o $@

$(X86_64_KERNEL_C_OBJ): kernel/arch/x86_64/kernel.c kernel/arch/x86_64/boot_context.h kernel/arch/x86_64/boot_info.h kernel/arch/x86_64/console.h kernel/arch/x86_64/cpuid.h kernel/arch/x86_64/gdt.h kernel/arch/x86_64/idt.h kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/paging.h kernel/arch/x86_64/paging_builder.h kernel/arch/x86_64/paging_plan.h kernel/arch/x86_64/pmm.h kernel/arch/x86_64/pmm_plan.h kernel/arch/x86_64/tss.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_CONSOLE_OBJ): kernel/arch/x86_64/console.c kernel/arch/x86_64/console.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_BOOT_INFO_OBJ): kernel/arch/x86_64/boot_info.c kernel/arch/x86_64/boot_info.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_BOOT_CONTEXT_OBJ): kernel/arch/x86_64/boot_context.c kernel/arch/x86_64/boot_context.h kernel/arch/x86_64/boot_info.h kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/pmm_plan.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_CPUID_OBJ): kernel/arch/x86_64/cpuid.c kernel/arch/x86_64/cpuid.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_GDT_OBJ): kernel/arch/x86_64/gdt.c kernel/arch/x86_64/gdt.h kernel/arch/x86_64/tss.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_TSS_OBJ): kernel/arch/x86_64/tss.c kernel/arch/x86_64/tss.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_MEMORY_LAYOUT_OBJ): kernel/arch/x86_64/memory_layout.c kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_PAGING_OBJ): kernel/arch/x86_64/paging.c kernel/arch/x86_64/paging.h kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_PAGING_PLAN_OBJ): kernel/arch/x86_64/paging_plan.c kernel/arch/x86_64/paging_plan.h kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/pmm_plan.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_PAGING_BUILDER_OBJ): kernel/arch/x86_64/paging_builder.c kernel/arch/x86_64/paging_builder.h kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/paging_plan.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_PMM_PLAN_OBJ): kernel/arch/x86_64/pmm_plan.c kernel/arch/x86_64/pmm_plan.h kernel/arch/x86_64/boot_info.h kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_PMM_OBJ): kernel/arch/x86_64/pmm.c kernel/arch/x86_64/pmm.h kernel/arch/x86_64/boot_info.h kernel/arch/x86_64/memory_layout.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_IDT_OBJ): kernel/arch/x86_64/idt.c kernel/arch/x86_64/idt.h kernel/arch/x86_64/console.h kernel/arch/x86_64/types.h | $(X86_64_BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_x86_64) -c $< -o $@

$(X86_64_KERNEL): linker.x86_64.ld $(X86_64_BOOT_OBJ)
	$(LD) $(LDFLAGS_x86_64) $(X86_64_BOOT_OBJ) -o $@
	$(GRUB_FILE) --is-x86-multiboot2 $@

$(X86_64_ISO): $(X86_64_KERNEL) $(X86_64_GRUB_CFG)
	rm -rf $(X86_64_ISO_DIR)
	mkdir -p $(X86_64_ISO_DIR)/boot/grub
	cp $(X86_64_KERNEL) $(X86_64_ISO_DIR)/boot/kernel.elf
	cp $(X86_64_GRUB_CFG) $(X86_64_ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(X86_64_ISO) $(X86_64_ISO_DIR)

$(BUILD_DIR)/kernel/userland/userland_images.o: kernel/userland/userland_images.asm $(USERLAND_FLAT_BIN) | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(USERLAND_FLAT_BIN) $(ASM_OBJ) $(C_OBJ)
	$(LD) $(LDFLAGS) $(ASM_OBJ) $(C_OBJ) -o $@

$(ISO): $(KERNEL) boot/grub.cfg
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.bin
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

run: $(ISO)
	$(QEMU) -boot d -cdrom $(ISO)

run-debug: debug
	$(QEMU) -boot d -cdrom $(ISO)

clean:
	rm -rf $(BUILD_DIR)
