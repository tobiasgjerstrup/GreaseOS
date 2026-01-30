# the-x86-project
I am bored.

## Kernel Project

A basic x86 kernel (Multiboot + C).

### Building the Kernel

**Requirements:**
- `nasm` - Netwide Assembler
- `gcc` + `ld` (32-bit capable)
- `grub-mkrescue` + `xorriso`
- `mkfs.fat` (from `dosfstools`)
- `make` (optional, for using Makefile)
- `qemu-system-i386` (optional, for running in emulator)

**Build commands:**
```bash
# Using make
make build          # Build GRUB ISO
make run            # Build and run in QEMU (creates build/disk.img if missing)
make clean          # Clean build files
```

### Files
- `entry.asm` - Multiboot entry stub
- `kernel.c` - C kernel entry (`kernel_main`) with shell
- `console.c` - VGA text console
- `drivers/ata.c` - ATA PIO disk I/O
- `fs/fat.c` - FAT16 filesystem
- `linker.ld` - Kernel linker script
- `grub/grub.cfg` - GRUB config
- `Makefile` - Build automation

### hello world
```bash
nasm -f elf64 ./sidequests/helloworld.asm -o ./build/app.o && ld ./build/app.o -o ./build/app && ./build/app
```
