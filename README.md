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
make vhd            # Convert build/disk.img to build/disk.vhd (Hyper-V)
make vhdx           # Convert build/disk.img to build/disk.vhdx (Hyper-V)
make clean          # Clean build files
```

### Hyper-V notes
- Use a Gen1 VM (legacy BIOS) with an IDE-attached data disk.
- The kernel expects a raw FAT16 disk image like build/disk.img. If you use Hyper-V, convert that image to a fixed VHD/VHDX and attach it as an IDE disk (IDE 0:1 is typical).
- If you attach a blank VHD/VHDX, you will see “FAT init failed: No boot sector”.

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
