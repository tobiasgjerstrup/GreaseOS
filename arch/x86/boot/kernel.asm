; x86 Kernel Main Code
; This is the main kernel that runs after boot.asm

bits 32
org 0x1000

kernel_start:
    ; Initialize video memory for output
    mov edi, 0xb8000  ; Video memory address
    xor eax, eax
    mov ecx, 2000     ; Clear entire screen
    rep stosd

    ; Print "Kernel Loaded" at top of screen
    mov edi, 0xb8000
    mov esi, kernel_msg
    xor ecx, ecx

.print_loop:
    movzx eax, byte [esi]
    cmp al, 0
    je .print_done
    mov byte [edi], al      ; Character
    mov byte [edi + 1], 0x0F ; White text on black
    add esi, 1
    add edi, 2
    jmp .print_loop

.print_done:
    ; Call main kernel function
    call kernel_main

    ; Halt
    cli
    hlt
    jmp $

kernel_main:
    ; Place your kernel code here
    push ebp
    mov ebp, esp

    ; TODO: Initialize IDT (Interrupt Descriptor Table)
    ; TODO: Initialize PIC (Programmable Interrupt Controller)
    ; TODO: Setup paging
    ; TODO: Load kernel modules

    mov esp, ebp
    pop ebp
    ret

; Data section
kernel_msg: db "Kernel Loaded Successfully!", 0

kernel_end:
