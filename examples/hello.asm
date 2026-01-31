; Example user program - Hello World
; Compile: nasm -f bin hello.asm -o hello.bin
; Run in OS: exec hello.bin

bits 32
org 0x00400000              ; 32-bit: loaded at 0x400000

section .text

syscall_entry:
    dd 0                    ; Filled by kernel with syscall handler address

_start:
    ; syscall_write(message)
    mov eax, 2              ; SYSCALL_WRITE
    mov ebx, message        ; arg1 = string
    xor ecx, ecx            ; arg2 = 0
    xor edx, edx            ; arg3 = 0
    call [syscall_entry]
    
    ; Return to kernel
    ret
    
message:
    db 'Hello from user program!', 0x0A, 0
