; Simple x86 Bootloader/Kernel
; Compiled with: nasm -f bin boot.asm -o boot.bin

org 0x7c00        ; Bootloader loads at 0x7c00
bits 16           ; 16-bit real mode

start:
    cli           ; Clear interrupt flag
    xor ax, ax    ; Clear ax
    mov ds, ax    ; Set data segment to 0
    mov es, ax    ; Set extra segment to 0
    mov ss, ax    ; Set stack segment to 0
    mov sp, 0x7c00 ; Set stack pointer

    ; Print welcome message
    mov si, welcome_msg
    call print_string

    ; Enable A20 line (required for accessing memory above 1MB)
    call enable_a20

    ; Load GDT (Global Descriptor Table)
    lgdt [gdt_descriptor]

    ; Enter protected mode
    mov eax, cr0
    or eax, 1       ; Set PE (Protected Mode Enable) bit
    mov cr0, eax

    ; Far jump to flush prefetch queue and set code segment
    jmp 0x08:protected_mode

bits 32           ; 32-bit protected mode
protected_mode:
    ; Set up data segments
    mov ax, 0x10   ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up stack
    mov esp, 0x90000

    ; Print success message in protected mode
    mov byte [0xb8000], 'K'  ; Write 'K' to video memory
    mov byte [0xb8001], 0x0F ; White text on black background
    mov byte [0xb8002], 'e'
    mov byte [0xb8003], 0x0F
    mov byte [0xb8004], 'r'
    mov byte [0xb8005], 0x0F
    mov byte [0xb8006], 'n'
    mov byte [0xb8007], 0x0F
    mov byte [0xb8008], 'e'
    mov byte [0xb8009], 0x0F
    mov byte [0xb800a], 'l'
    mov byte [0xb800b], 0x0F

    ; Infinite loop
    hlt
    jmp $

bits 16           ; Back to 16-bit for helper functions

; Enable A20 line using BIOS interrupt
enable_a20:
    mov ax, 0x2401
    int 0x15
    ret

; Print string in real mode
; Input: si = pointer to string (null-terminated)
print_string:
    push ax
    xor ax, ax
.loop:
    lodsb               ; Load byte from [si] into al, increment si
    or al, al           ; Check if null terminator
    jz .done
    mov ah, 0x0e        ; BIOS teletype function
    int 0x10
    jmp .loop
.done:
    pop ax
    ret

; Data section for real mode
welcome_msg: db "Booting x86 Kernel...", 0x0d, 0x0a, 0

; GDT (Global Descriptor Table)
gdt_start:
    ; Null descriptor (required)
    dq 0x0

    ; Code segment descriptor
    dw 0xffff       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 0x9a         ; Access byte (present, ring 0, code, execute/read)
    db 0xcf         ; Flags and limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

    ; Data segment descriptor
    dw 0xffff       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 0x92         ; Access byte (present, ring 0, data, read/write)
    db 0xcf         ; Flags and limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

gdt_end:

; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                ; Offset

; Boot signature (required for bootloader)
times 510-($-$$) db 0   ; Pad with zeros
dw 0xaa55               ; Boot signature
