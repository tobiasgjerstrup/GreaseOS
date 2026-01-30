# the-x86-project
I am bored.

## Kernel Project

A basic x86 bootloader and kernel written in assembly.

### Building the Kernel

**Requirements:**
- `nasm` - Netwide Assembler
- `make` (optional, for using Makefile)
- `qemu-system-i386` (optional, for running in emulator)

**Build commands:**
```bash
# Using make
make build          # Build kernel image
make run            # Build and run in QEMU
make clean          # Clean build files

# Manual build
nasm -f bin boot.asm -o boot.bin
nasm -f bin kernel.asm -o kernel.bin
cat boot.bin kernel.bin > build/kernel.img
```

**Run in QEMU:**
```bash
qemu-system-i386 -drive format=raw,file=build/kernel.img
```

### Files
- `boot.asm` - Bootloader code (16-bit real mode → 32-bit protected mode)
- `kernel.asm` - Main kernel code
- `Makefile` - Build automation

### hello world
```bash
nasm -f elf64 ./sidequests/helloworld.asm -o ./build/app.o && ld ./build/app.o -o ./build/app && ./build/app
```
