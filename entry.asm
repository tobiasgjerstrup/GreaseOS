; Multiboot entry stub (32-bit)

bits 32

section .multiboot
align 4
    dd 0x1BADB002            ; multiboot magic
    dd 0x00000003            ; flags (align + mem info)
    dd -(0x1BADB002 + 0x00000003) ; checksum

section .text
align 16

global start
extern kernel_main

start:
    cli
    mov esp, stack_top
    call kernel_main

.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
