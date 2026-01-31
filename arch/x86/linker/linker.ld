ENTRY(start)

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    .multiboot : {
        *(.multiboot)
    }

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(COMMON)
        *(.bss*)
    }

    __kernel_end = .;
}
