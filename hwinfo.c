#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "hwinfo.h"
#include "io.h"

static void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "a"(code));
}

static void u32_to_str(uint32_t value, char* out, size_t out_len)
{
    if (out_len == 0)
    {
        return;
    }

    char temp[16];
    size_t idx = 0;
    if (value == 0)
    {
        temp[idx++] = '0';
    }
    else
    {
        while (value > 0 && idx < sizeof(temp))
        {
            temp[idx++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    size_t out_idx = 0;
    while (idx > 0 && out_idx + 1 < out_len)
    {
        out[out_idx++] = temp[--idx];
    }
    out[out_idx] = '\0';
}

uint32_t hwinfo_get_memory_kb(void)
{
    outb(0x70, 0x17);
    uint32_t low = inb(0x71);
    outb(0x70, 0x18);
    uint32_t high = inb(0x71);
    return (high << 8) | low;
}

static void get_cpu_vendor(char* vendor, size_t len)
{
    if (len < 13)
    {
        return;
    }

    uint32_t eax, ebx, ecx, edx;
    cpuid(0, &eax, &ebx, &ecx, &edx);

    vendor[0] = (char)(ebx & 0xFF);
    vendor[1] = (char)((ebx >> 8) & 0xFF);
    vendor[2] = (char)((ebx >> 16) & 0xFF);
    vendor[3] = (char)((ebx >> 24) & 0xFF);

    vendor[4] = (char)(edx & 0xFF);
    vendor[5] = (char)((edx >> 8) & 0xFF);
    vendor[6] = (char)((edx >> 16) & 0xFF);
    vendor[7] = (char)((edx >> 24) & 0xFF);

    vendor[8] = (char)(ecx & 0xFF);
    vendor[9] = (char)((ecx >> 8) & 0xFF);
    vendor[10] = (char)((ecx >> 16) & 0xFF);
    vendor[11] = (char)((ecx >> 24) & 0xFF);

    vendor[12] = '\0';
}

static void get_cpu_features(uint32_t* features_ecx, uint32_t* features_edx)
{
    uint32_t eax, ebx;
    cpuid(1, &eax, &ebx, features_ecx, features_edx);
}

void hwinfo_display(void)
{
    char buf[64];
    
    console_write("=== Hardware Information ===\n\n");

    char vendor[13];
    get_cpu_vendor(vendor, sizeof(vendor));
    console_write("CPU Vendor: ");
    console_write(vendor);
    console_putc('\n');

    uint32_t feat_ecx, feat_edx;
    get_cpu_features(&feat_ecx, &feat_edx);

    console_write("CPU Features: ");
    if (feat_edx & (1 << 0))
    {
        console_write("FPU ");
    }
    if (feat_edx & (1 << 4))
    {
        console_write("TSC ");
    }
    if (feat_edx & (1 << 5))
    {
        console_write("MSR ");
    }
    if (feat_edx & (1 << 23))
    {
        console_write("MMX ");
    }
    if (feat_edx & (1 << 25))
    {
        console_write("SSE ");
    }
    if (feat_edx & (1 << 26))
    {
        console_write("SSE2 ");
    }
    if (feat_ecx & (1 << 0))
    {
        console_write("SSE3 ");
    }
    console_putc('\n');

    uint32_t mem_kb = hwinfo_get_memory_kb();
    uint32_t mem_mb = mem_kb / 1024;
    console_write("Memory: ");
    u32_to_str(mem_kb, buf, sizeof(buf));
    console_write(buf);
    console_write(" KB (");
    u32_to_str(mem_mb, buf, sizeof(buf));
    console_write(buf);
    console_write(" MB)\n");

    console_write("\n=== System Resources ===\n\n");
    
    extern uint8_t __kernel_start;
    extern uint8_t __kernel_end;
    uint32_t kernel_size = (uint32_t)(&__kernel_end - &__kernel_start);
    uint32_t kernel_kb = kernel_size / 1024;
    
    console_write("Kernel size: ");
    u32_to_str(kernel_kb, buf, sizeof(buf));
    console_write(buf);
    console_write(" KB\n");

    console_write("VGA memory: 0xB8000 (80x25 text mode)\n");
}
