.PHONY: all clean build run help

all: build

BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/isodir
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/kernel.iso
ISO_TMP := /tmp/kernel.iso
DISK_IMG := $(BUILD_DIR)/disk.img
DISK_SIZE_MB := 16
VHD_IMG := $(BUILD_DIR)/disk.vhd
VHDX_IMG := $(BUILD_DIR)/disk.vhdx

NASMFLAGS := -f elf32
CFLAGS := -m32 -ffreestanding -fno-pie -no-pie -fno-stack-protector -O2 -Wall -Wextra -I.
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

C_SOURCES := kernel.c console.c drivers/ata.c fs/fat.c
C_OBJS := $(C_SOURCES:%.c=$(BUILD_DIR)/%.o)

build: $(ISO)

$(BUILD_DIR)/entry.o: entry.asm | $(BUILD_DIR)/.dir
	nasm $(NASMFLAGS) entry.asm -o $(BUILD_DIR)/entry.o

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/.dir
	@mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(BUILD_DIR)/entry.o $(C_OBJS) linker.ld | $(BUILD_DIR)/.dir
	ld $(LDFLAGS) -o $(KERNEL_ELF) $(BUILD_DIR)/entry.o $(C_OBJS)

$(ISO): $(KERNEL_ELF) grub/grub.cfg | $(BUILD_DIR)/.dir
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@rm -f $(ISO_TMP)
	grub-mkrescue -o $(ISO_TMP) $(ISO_DIR)
	cp $(ISO_TMP) $(ISO)

$(DISK_IMG): | $(BUILD_DIR)/.dir
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=$(DISK_SIZE_MB)
	mkfs.fat -F 16 -I $(DISK_IMG)

$(BUILD_DIR)/.dir:
	mkdir -p $(BUILD_DIR)
	@echo "Created $(BUILD_DIR)"
	@touch $(BUILD_DIR)/.dir

run: build $(DISK_IMG)
	@echo "Running kernel in QEMU..."
	qemu-system-i386 -boot d -cdrom $(ISO) -drive file=$(DISK_IMG),format=raw,if=ide

vhd: $(DISK_IMG)
	@echo "Converting raw disk to VHD..."
	qemu-img convert -f raw -O vpc $(DISK_IMG) $(VHD_IMG)

vhdx: $(DISK_IMG)
	@echo "Converting raw disk to VHDX..."
	qemu-img convert -f raw -O vhdx $(DISK_IMG) $(VHDX_IMG)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build files"

help:
	@echo "x86 Kernel Build System (Multiboot + C)"
	@echo "make build - Build GRUB ISO"
	@echo "make run   - Build and run in QEMU"
	@echo "make vhd   - Convert build/disk.img to build/disk.vhd"
	@echo "make vhdx  - Convert build/disk.img to build/disk.vhdx"
	@echo "make clean - Remove build files"
