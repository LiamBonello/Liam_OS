BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso
USERLAND_BUILD_DIR := $(BUILD_DIR)/userland
KERNEL := $(BUILD_DIR)/kernel.bin
ISO := $(BUILD_DIR)/liam_os.iso

ARCH ?= i386
SUPPORTED_ARCHES := i386 x86_64
ARCH_DIR := kernel/arch/$(ARCH)

ASM := nasm
CC := clang
LD := ld.lld
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
$(error ARCH=x86_64 is scaffolded but not bootable yet. Use 'make x86_64-info' for the staged migration plan.)
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

.PHONY: all clean run run-debug debug check-tools userland x86_64-info

all: $(ISO)

debug: CFLAGS := $(BASE_CFLAGS) -DLIAM_OS_DEBUG
debug: clean $(ISO)

check-tools:
	@command -v $(ASM) >/dev/null || (echo "Missing tool: $(ASM)" && exit 1)
	@command -v $(CC) >/dev/null || (echo "Missing tool: $(CC)" && exit 1)
	@command -v $(LD) >/dev/null || (echo "Missing tool: $(LD)" && exit 1)
	@command -v grub-mkrescue >/dev/null || (echo "Missing tool: grub-mkrescue" && exit 1)
	@command -v $(QEMU) >/dev/null || (echo "Missing tool: $(QEMU)" && exit 1)

x86_64-info:
	@echo "Liam_OS x86_64 path is scaffolded but not bootable yet."
	@echo "Current bootable default remains: ARCH=i386"
	@echo "Next milestone: add linker.x86_64.ld and a long-mode bootstrap under kernel/arch/x86_64/."
	@echo "See: kernel/arch/x86_64/README.md and ../docs/core/x86_64-migration-plan.md"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(USERLAND_BUILD_DIR): | $(BUILD_DIR)
	mkdir -p $(USERLAND_BUILD_DIR)

userland: $(USERLAND_FLAT_BIN)

define BUILD_USERLAND_FLAT
$(USERLAND_BUILD_DIR)/$(1).bin: userland/flat/$(1)/$(1).asm | $(USERLAND_BUILD_DIR)
	$$(ASM) -f bin $$< -o $$@
endef

$(foreach name,$(USERLAND_FLAT_NAMES),$(eval $(call BUILD_USERLAND_FLAT,$(name))))

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
