; Multiboot2 header for 64-bit
section .multiboot
align 8
multiboot2_header_start:
    dd 0xE85250D6                   ; Multiboot2 magic
    dd 0                            ; architecture (i386)
    dd multiboot2_header_end - multiboot2_header_start
    dd -(0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header_start))

    ; End tag
align 8
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
multiboot2_header_end:

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
stack_bottom:
    resb 16384
align 16
stack_top:
align 16
idt64:
    resb 4096

section .text
bits 32
global start
extern kernel_main

start:
    cli
    mov esp, stack_top

    ; Check for CPUID
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_long_mode

    ; Check for long mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    ; Set up page tables
    ; Map first 2MB
    mov eax, p3_table
    or eax, 0b11
    mov [p4_table], eax

    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table

    ; Load P4 to CR3
    mov eax, p4_table
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

.no_long_mode:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov dword [0xb800c], 0x4f6f4f4e
    mov dword [0xb8010], 0x4f4c4f20
    mov dword [0xb8014], 0x4f6e4f6f
    mov dword [0xb8018], 0x4f204f67
    mov dword [0xb801c], 0x4f6f4f4d
    mov dword [0xb8020], 0x4f654f64
    hlt

section .rodata
gdt64:
    dq 0
.code: equ $ - gdt64
    dq 0x00AF9A000000FFFF
.data: equ $ - gdt64
    dq 0x00AF92000000FFFF
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
align 8
idt64_ptr:
    dw 4096 - 1
    dq idt64

section .text
bits 64
isr_stub:
    cli
.halt:
    hlt
    jmp .halt

long_mode_start:
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rsp, stack_top
    and rsp, 0xFFFFFFFFFFFFFFF0
    sub rsp, 8

    lea rdi, [idt64]
    mov rcx, 256
    mov rax, isr_stub
    mov rbx, rax
    shr rbx, 16
    mov dx, bx
    mov r8, rax
    shr r8, 32
.idt_fill:
    mov word [rdi + 0], ax
    mov word [rdi + 2], gdt64.code
    mov byte [rdi + 4], 0
    mov byte [rdi + 5], 0x8E
    mov word [rdi + 6], dx
    mov dword [rdi + 8], r8d
    mov dword [rdi + 12], 0
    add rdi, 16
    loop .idt_fill

    lidt [idt64_ptr]

    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
