section .bss
    ; variables

section .data
    ; constants
    hello: db "Hello World!", 10; db = define byte, 10 = \n
    helloLen: equ $-hello ;equ = create constant, $ = current address, $-hello = number of bytes between current address and start of string

section .text
    global _start ;entry point for linker

    _start: ;code starts executing here
        mov rax,1 ; sys_write
        mov rdi,1 ; stdout
        mov rsi,hello ; message to write
        mov rdx,helloLen
        syscall

        ;end program
        mov rax,60 ;sys_exit
        mov rdi,0   ;error_code
        syscall