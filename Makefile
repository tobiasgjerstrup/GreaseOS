.PHONY: all clean build run help

all: build

BUILD_DIR := build

build: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	@echo "Building kernel..."
	@cat $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin > $(BUILD_DIR)/kernel.img

$(BUILD_DIR)/boot.bin: boot.asm | $(BUILD_DIR)/.dir
	nasm -f bin boot.asm -o $(BUILD_DIR)/boot.bin

$(BUILD_DIR)/kernel.bin: kernel.asm | $(BUILD_DIR)/.dir
	nasm -f bin kernel.asm -o $(BUILD_DIR)/kernel.bin


$(BUILD_DIR)/.dir:
	mkdir -p $(BUILD_DIR)
	@echo "Created $(BUILD_DIR)"
	@touch $(BUILD_DIR)/.dir

run: build
	@echo "Running kernel in QEMU..."
	qemu-system-i386 -drive format=raw,file=$(BUILD_DIR)/kernel.img

clean:
	rm -f $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.img
	@echo "Cleaned build files"

help:
	@echo "x86 Kernel Build System"
	@echo "make build - Build the kernel"
	@echo "make run   - Build and run in QEMU"
	@echo "make clean - Remove build files"
