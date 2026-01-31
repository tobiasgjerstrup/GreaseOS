; Example user program - Hello World (64-bit)
; Compile: nasm -f bin hello64.asm -o hello64.bin
; Run in OS: exec hello64.bin

bits 64
org 0x00010000              ; Binary will be loaded at this address (64-bit mode)

section .text

syscall_entry:
    dq 0                    ; Filled by kernel with syscall handler address

_start:
    ; syscall_write(message)
    mov rax, 2              ; SYSCALL_WRITE
    lea rbx, [rel message]  ; arg1 = string (position-independent)
    xor rcx, rcx            ; arg2 = 0
    xor rdx, rdx            ; arg3 = 0
    call [rel syscall_entry]
    
    ; Return to kernel
    ret
    
message:
    db 'Hello from 64-bit user program!', 0x0A, 0
