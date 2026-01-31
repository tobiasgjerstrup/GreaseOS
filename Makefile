.PHONY: all clean build build64 run run64 help

all: build

BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/isodir
ISO_DIR_64 := $(BUILD_DIR)/isodir64
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_ELF_64 := $(BUILD_DIR)/kernel64.elf
ISO := $(BUILD_DIR)/kernel.iso
ISO_64 := $(BUILD_DIR)/kernel64.iso
ISO_TMP := /tmp/kernel.iso
ISO_TMP_64 := /tmp/kernel64.iso
DISK_IMG := $(BUILD_DIR)/disk.img
DISK_SIZE_MB := 16
VHD_IMG := $(BUILD_DIR)/disk.vhd
VHDX_IMG := $(BUILD_DIR)/disk.vhdx

NASMFLAGS_32 := -f elf32
NASMFLAGS_64 := -f elf64
CFLAGS_32 := -m32 -ffreestanding -fno-pie -no-pie -fno-stack-protector -O2 -Wall -Wextra -I.
CFLAGS_64 := -m64 -ffreestanding -fno-pie -no-pie -fno-stack-protector -O2 -Wall -Wextra -I. -mcmodel=large -mno-red-zone
LDFLAGS_32 := -m elf_i386 -T linker.ld -nostdlib
LDFLAGS_64 := -m elf_x86_64 -T linker64.ld -nostdlib

C_SOURCES := kernel.c console.c keyboard.c editor.c hwinfo.c exec.c snake.c clipboard.c drivers/ata.c fs/fat.c
C_OBJS_32 := $(C_SOURCES:%.c=$(BUILD_DIR)/32/%.o)
C_OBJS_64 := $(C_SOURCES:%.c=$(BUILD_DIR)/64/%.o)

build: $(ISO)

build64: $(ISO_64)

$(BUILD_DIR)/entry32.o: entry.asm | $(BUILD_DIR)/.dir
	nasm $(NASMFLAGS_32) entry.asm -o $(BUILD_DIR)/entry32.o

$(BUILD_DIR)/entry64.o: entry64.asm | $(BUILD_DIR)/.dir
	nasm $(NASMFLAGS_64) entry64.asm -o $(BUILD_DIR)/entry64.o

$(BUILD_DIR)/32/%.o: %.c | $(BUILD_DIR)/.dir
	@mkdir -p $(dir $@)
	gcc $(CFLAGS_32) -c $< -o $@

$(BUILD_DIR)/64/%.o: %.c | $(BUILD_DIR)/.dir
	@mkdir -p $(dir $@)
	gcc $(CFLAGS_64) -c $< -o $@

$(KERNEL_ELF): $(BUILD_DIR)/entry32.o $(C_OBJS_32) linker.ld | $(BUILD_DIR)/.dir
	ld $(LDFLAGS_32) -o $(KERNEL_ELF) $(BUILD_DIR)/entry32.o $(C_OBJS_32)

$(KERNEL_ELF_64): $(BUILD_DIR)/entry64.o $(C_OBJS_64) linker64.ld | $(BUILD_DIR)/.dir
	ld $(LDFLAGS_64) -o $(KERNEL_ELF_64) $(BUILD_DIR)/entry64.o $(C_OBJS_64)

$(ISO): $(KERNEL_ELF) grub/grub.cfg | $(BUILD_DIR)/.dir
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@rm -f $(ISO_TMP)
	grub-mkrescue -o $(ISO_TMP) $(ISO_DIR)
	cp $(ISO_TMP) $(ISO)

$(ISO_64): $(KERNEL_ELF_64) grub/grub64.cfg | $(BUILD_DIR)/.dir
	mkdir -p $(ISO_DIR_64)/boot/grub
	cp $(KERNEL_ELF_64) $(ISO_DIR_64)/boot/kernel.elf
	cp grub/grub64.cfg $(ISO_DIR_64)/boot/grub/grub.cfg
	@rm -f $(ISO_TMP_64)
	grub-mkrescue -o $(ISO_TMP_64) $(ISO_DIR_64)
	cp $(ISO_TMP_64) $(ISO_64)

$(DISK_IMG): | $(BUILD_DIR)/.dir
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=$(DISK_SIZE_MB)
	mkfs.fat -F 16 -I $(DISK_IMG)

$(BUILD_DIR)/.dir:
	mkdir -p $(BUILD_DIR)
	@echo "Created $(BUILD_DIR)"
	@touch $(BUILD_DIR)/.dir

run: build $(DISK_IMG)
	@echo "Running 32-bit kernel in QEMU..."
	qemu-system-i386 -boot d -cdrom $(ISO) -drive file=$(DISK_IMG),format=raw,if=ide

run64: build64 $(DISK_IMG)
	@echo "Running 64-bit kernel in QEMU..."
	qemu-system-x86_64 -boot d -cdrom $(ISO_64) -drive file=$(DISK_IMG),format=raw,if=ide

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
	@echo "make build   - Build 32-bit GRUB ISO"
	@echo "make build64 - Build 64-bit GRUB ISO"
	@echo "make run     - Build and run 32-bit in QEMU"
	@echo "make run64   - Build and run 64-bit in QEMU"
	@echo "make vhd     - Convert build/disk.img to build/disk.vhd"
	@echo "make vhdx    - Convert build/disk.img to build/disk.vhdx"
	@echo "make clean   - Remove build files"
