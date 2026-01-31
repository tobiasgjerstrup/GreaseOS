# GreaseOS previously known as the-x86-project
I am bored.

## GreaseOS

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
### Shell Commands
- `help` - List available commands
- `ls`, `cd`, `pwd`, `mkdir`, `touch` - File system operations
- `rmdir`, `rm`, `cp` - Remove/copy files or directories
- `cat <file>`, `write <file> <text>`, `echo <text>` - Read/write/print text
- `v <file>` - Open full-screen text editor
- `paste` - Paste clipboard contents into the editor
- `exec <file>` - Execute a flat binary program (no ELF yet)
- `info`, `hw` - Show kernel and hardware information
- `df` - Show disk usage
- `snake` - Launch the snake game
- `ss` - Show simple system stats
- `clear` - Clear screen
- `shutdown`, `restart` - Power control

### Editor Notes
- `v <file>` opens a full-screen editor with a status line.
- `Ctrl+S` saves, `Ctrl+Q` quits, and `Ctrl+V` pastes from the clipboard.
- Use `paste` from the shell to load clipboard contents into the editor.

### Running Programs
See [tools/examples/README.md](tools/examples/README.md) for how to write and compile user programs (flat binaries only).

**Note:** `exec` currently supports only flat binaries (raw `.bin`); ELF loading is not implemented yet.

Quick example:
```bash
# Compile example program
nasm -f bin examples/hello.asm -o hello.bin

# Mount disk and copy binary
sudo mount -o loop build/disk.img /tmp/disk
sudo cp hello.bin /tmp/disk/
sudo umount /tmp/disk

# Run in OS
make run
# In OS shell: exec hello.bin
```
### Files
- `entry.asm` - Multiboot entry stub (32-bit)
- `entry64.asm` - Multiboot2 entry stub with long mode setup (64-bit)
- `kernel.c` - C kernel entry (`kernel_main`) with shell
- `console.c` - VGA text console
- `keyboard.c` - Keyboard input with arrow keys and Ctrl support
- `editor.c` - Full-screen text editor (`v` command)
- `clipboard.c` - Clipboard support used by `paste`
- `snake.c` - Snake game (`snake` command)
- `hwinfo.c` - Hardware information display (`hw` command)
- `exec.c` - Binary execution engine with syscall interface
- `drivers/ata.c` - ATA PIO disk I/O
- `fs/fat.c` - FAT16 filesystem with multi-cluster support
- `linker.ld` - Kernel linker script (32-bit)
- `linker64.ld` - Kernel linker script (64-bit)
- `grub/grub.cfg` - GRUB config (32-bit)
- `grub/grub64.cfg` - GRUB config (64-bit)
- `Makefile` - Build automation