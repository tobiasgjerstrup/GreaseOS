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
make build          # Build 32-bit GRUB ISO (build/kernel.iso)
make build64        # Build 64-bit GRUB ISO (build/kernel64.iso)
make run            # Build and run 32-bit in QEMU
make run64          # Build and run 64-bit in QEMU
make vhd            # Convert build/disk.img to build/disk.vhd (Hyper-V)
make vhdx           # Convert build/disk.img to build/disk.vhdx (Hyper-V)
make clean          # Clean build files
```

**Architecture Support:**
- **32-bit (i386)**: Standard Multiboot, runs on any x86 CPU
- **64-bit (x86_64)**: Multiboot2 with long mode, requires 64-bit CPU

**For USB boot:**
- 32-bit: `sudo dd if=build/kernel.iso of=/dev/sdX bs=4M status=progress oflag=sync`
- 64-bit: `sudo dd if=build/kernel64.iso of=/dev/sdX bs=4M status=progress oflag=sync`
- (Replace `/dev/sdX` with your USB device)

### Hyper-V notes
- Use a Gen1 VM (legacy BIOS) with an IDE-attached data disk.
- The kernel expects a raw FAT16 disk image like build/disk.img. If you use Hyper-V, convert that image to a fixed VHD/VHDX and attach it as an IDE disk (IDE 0:1 is typical).
- If you attach a blank VHD/VHDX, you will see “FAT init failed: No boot sector”.

### Files
- `entry.asm` - Multiboot entry stub (32-bit)
- `entry64.asm` - Multiboot2 entry stub with long mode setup (64-bit)
- `kernel.c` - C kernel entry (`kernel_main`) with shell
- `console.c` - VGA text console
- `keyboard.c` - Keyboard input with arrow keys and Ctrl support
- `editor.c` - Full-screen text editor (`v` command)
- `hwinfo.c` - Hardware information display (`hw` command)
- `drivers/ata.c` - ATA PIO disk I/O
- `fs/fat.c` - FAT16 filesystem with multi-cluster support
- `linker.ld` - Kernel linker script (32-bit)
- `linker64.ld` - Kernel linker script (64-bit)
- `grub/grub.cfg` - GRUB config (32-bit)
- `grub/grub64.cfg` - GRUB config (64-bit)
- `Makefile` - Build automation

### hello world
```bash
nasm -f elf64 ./sidequests/helloworld.asm -o ./build/app.o && ld ./build/app.o -o ./build/app && ./build/app
```
