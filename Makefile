.PHONY: all clean build run help

all: build

BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/isodir
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/kernel.iso

NASMFLAGS := -f elf32
CFLAGS := -m32 -ffreestanding -fno-pie -no-pie -fno-stack-protector -O2 -Wall -Wextra
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

build: $(ISO)

$(BUILD_DIR)/entry.o: entry.asm | $(BUILD_DIR)/.dir
	nasm $(NASMFLAGS) entry.asm -o $(BUILD_DIR)/entry.o

$(BUILD_DIR)/kernel.o: kernel.c | $(BUILD_DIR)/.dir
	gcc $(CFLAGS) -c kernel.c -o $(BUILD_DIR)/kernel.o

$(KERNEL_ELF): $(BUILD_DIR)/entry.o $(BUILD_DIR)/kernel.o linker.ld | $(BUILD_DIR)/.dir
	ld $(LDFLAGS) -o $(KERNEL_ELF) $(BUILD_DIR)/entry.o $(BUILD_DIR)/kernel.o

$(ISO): $(KERNEL_ELF) grub/grub.cfg | $(BUILD_DIR)/.dir
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

$(BUILD_DIR)/.dir:
	mkdir -p $(BUILD_DIR)
	@echo "Created $(BUILD_DIR)"
	@touch $(BUILD_DIR)/.dir

run: build
	@echo "Running kernel in QEMU..."
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build files"

help:
	@echo "x86 Kernel Build System (Multiboot + C)"
	@echo "make build - Build GRUB ISO"
	@echo "make run   - Build and run in QEMU"
	@echo "make clean - Remove build files"
