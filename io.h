#pragma once

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define ASM_VOLATILE(...) __asm__ __volatile__(__VA_ARGS__)
#else
#define ASM_VOLATILE(...)
#endif

static inline void outb(uint16_t port, uint8_t value)
{
    ASM_VOLATILE("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    ASM_VOLATILE("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value)
{
    ASM_VOLATILE("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t value;
    ASM_VOLATILE("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void)
{
    ASM_VOLATILE("outb %%al, $0x80" : : "a"(0));
}
