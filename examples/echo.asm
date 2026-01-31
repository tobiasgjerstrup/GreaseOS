; Interactive example - Echo program
; Compile: nasm -f bin echo.asm -o echo.bin
; Run in OS: exec echo.bin

bits 32
org 0x00400000              ; 32-bit: loaded at 0x400000

section .text

syscall_entry:
    dd 0                    ; Filled by kernel

_start:
    ; Print prompt
    mov eax, 2              ; SYSCALL_WRITE
    mov ebx, prompt
    xor ecx, ecx
    xor edx, edx
    call [syscall_entry]
    
    ; Read line
    mov eax, 4              ; SYSCALL_READ_LINE
    mov ebx, buffer
    mov ecx, 128
    xor edx, edx
    call [syscall_entry]
    
    ; Print what was entered
    mov eax, 2              ; SYSCALL_WRITE
    mov ebx, echo_msg
    xor ecx, ecx
    xor edx, edx
    call [syscall_entry]
    
    mov eax, 2              ; SYSCALL_WRITE
    mov ebx, buffer
    xor ecx, ecx
    xor edx, edx
    call [syscall_entry]
    
    mov eax, 1              ; SYSCALL_PUTC
    mov ebx, 0x0A           ; newline
    xor ecx, ecx
    xor edx, edx
    call [syscall_entry]
    
    ret

prompt:
    db 'Enter text: ', 0

echo_msg:
    db 'You entered: ', 0

buffer:
    times 128 db 0
