# User Program Examples

This directory contains example programs that can be compiled and run in the kernel using the `exec` command.

## Available Syscalls

Programs can call kernel functions using syscalls. The syscall handler address is stored at the beginning of the binary (first 4 bytes for 32-bit, first 8 bytes for 64-bit) and is filled in by the kernel when the program is loaded.

### Syscall Interface
Call the syscall handler with:
- **eax/rax**: Syscall number
- **ebx/rbx**: Argument 1
- **ecx/rcx**: Argument 2
- **edx/rdx**: Argument 3

### Syscall Numbers
- `1` - SYSCALL_PUTC: Print a character
  - arg1: Character to print
- `2` - SYSCALL_WRITE: Print a null-terminated string
  - arg1: Pointer to string
- `3` - SYSCALL_GETC: Read a character
  - arg1: Pointer to store character
- `4` - SYSCALL_READ_LINE: Read a line of input
  - arg1: Buffer pointer
  - arg2: Buffer size

## Compiling Examples

### 32-bit programs
```bash
nasm -f bin hello.asm -o hello.bin
nasm -f bin echo.asm -o echo.bin
```

### 64-bit programs
```bash
nasm -f bin hello64.asm -o hello64.bin
```

## Running in the OS

1. Build and run the kernel:
```bash
make clean
make run      # for 32-bit
# or
make run64    # for 64-bit
```

2. Copy the binary to the virtual disk (from Linux/WSL):
```bash
# Mount the disk image
mkdir -p /tmp/disk
sudo mount -o loop build/disk.img /tmp/disk

# Copy binary
sudo cp hello.bin /tmp/disk/

# Unmount
sudo umount /tmp/disk
```

3. In the OS shell, run:
```
exec hello.bin
```

## Writing Your Own Programs

Your program should:
1. Use `bits 32` or `bits 64` directive
2. Reserve the first 4/8 bytes for syscall handler pointer
3. Start code after the syscall pointer
4. Return with `ret` instruction

### 32-bit Template
```asm
bits 32

section .text

syscall_entry:
    dd 0        ; Filled by kernel

_start:
    ; Your code here
    ; Call syscalls like:
    ; mov eax, 2
    ; mov ebx, message
    ; call [syscall_entry]
    ret

message:
    db 'Hello!', 0
```

### 64-bit Template
```asm
bits 64

section .text

syscall_entry:
    dq 0        ; Filled by kernel

_start:
    ; Your code here
    ; Use position-independent code:
    ; lea rbx, [rel message]
    ret

message:
    db 'Hello!', 0
```

## Notes
- Programs run in kernel space (no memory protection)
- Maximum binary size: 64KB
- Use flat binary format (not ELF)
- Programs must be position-independent or assembled for loading at address 0
