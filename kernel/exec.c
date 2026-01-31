#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "exec.h"
#include "fs/fat.h"
#include "keyboard.h"

#define MAX_BINARY_SIZE 65536
#if defined(__x86_64__) || defined(__amd64__)
#define BINARY_LOAD_ADDR ((uint8_t*)0x00010000)  /* 64-bit: safe address within mapped range */
#else
#define BINARY_LOAD_ADDR ((uint8_t*)0x00400000)  /* 32-bit: standard user space address */
#endif
#define SYSCALL_PUTC 1
#define SYSCALL_WRITE 2
#define SYSCALL_GETC 3
#define SYSCALL_READ_LINE 4

typedef void (*binary_entry_t)(void);

static void syscall_handler_c(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    (void)arg3;
    
    switch (num)
    {
        case SYSCALL_PUTC:
            console_putc((char)arg1);
            break;
            
        case SYSCALL_WRITE:
            if (arg1 != 0)
            {
                console_write((const char*)(size_t)arg1);
            }
            break;
            
        case SYSCALL_GETC:
            if (arg1 != 0)
            {
                while (!keyboard_has_data())
                {
                }
                int key = keyboard_read_key();
                if (key > 0 && key < 256)
                {
                    console_putc((char)key);
                    *(char*)(size_t)arg1 = (char)key;
                }
            }
            break;
            
        case SYSCALL_READ_LINE:
            if (arg1 != 0 && arg2 > 0)
            {
                char* buffer = (char*)(size_t)arg1;
                size_t max_len = (size_t)arg2;
                size_t len = 0;
                
                while (len + 1 < max_len)
                {
                    while (!keyboard_has_data())
                    {
                    }
                    
                    int key = keyboard_read_key();
                    if (key == 0)
                    {
                        continue;
                    }
                    
                    if (key == '\n')
                    {
                        console_putc('\n');
                        buffer[len] = '\0';
                        return;
                    }
                    
                    if (key == '\b')
                    {
                        if (len > 0)
                        {
                            len--;
                            console_backspace();
                        }
                        continue;
                    }
                    
                    if (key >= 32 && key <= 126)
                    {
                        buffer[len++] = (char)key;
                        console_putc((char)key);
                    }
                }
                buffer[len] = '\0';
            }
            break;
    }
}

#if defined(__x86_64__) || defined(__amd64__)
static uint8_t syscall_handler_stub[64] = {
    0x50,                           // push rax
    0x53,                           // push rbx
    0x51,                           // push rcx
    0x52,                           // push rdx
    0x48, 0x89, 0xC7,               // mov rdi, rax (arg1 = syscall num)
    0x48, 0x89, 0xDE,               // mov rsi, rbx (arg2)
    0x48, 0x89, 0xCA,               // mov rdx, rcx (arg3)
    0x48, 0x89, 0xD1,               // mov rcx, rdx (arg4)
    0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,  // movabs rax, <handler_addr>
    0xFF, 0xD0,                     // call rax
    0x5A,                           // pop rdx
    0x59,                           // pop rcx
    0x5B,                           // pop rbx
    0x58,                           // pop rax
    0xC3                            // ret
};
#else
static uint8_t syscall_handler_stub[32] = {
    0x50,                           // push eax
    0x53,                           // push ebx
    0x51,                           // push ecx
    0x52,                           // push edx
    0x52,                           // push edx (arg4)
    0x51,                           // push ecx (arg3)
    0x53,                           // push ebx (arg2)
    0x50,                           // push eax (arg1 = syscall num)
    0xB8, 0, 0, 0, 0,               // mov eax, <handler_addr>
    0xFF, 0xD0,                     // call eax
    0x83, 0xC4, 0x10,               // add esp, 16 (clean up 4 args)
    0x5A,                           // pop edx
    0x59,                           // pop ecx
    0x5B,                           // pop ebx
    0x58,                           // pop eax
    0xC3                            // ret
};
#endif

int exec_run(const char* filename)
{
    console_write("exec: loading ");
    console_write(filename);
    console_write("...\n");
    
    size_t size = 0;
    if (fat_read(filename, (char*)BINARY_LOAD_ADDR, MAX_BINARY_SIZE, &size) != 0)
    {
        console_write("exec: ");
        console_write(fat_last_error());
        console_putc('\n');
        return -1;
    }
    
    console_write("exec: loaded ");
    console_write(filename);
    console_write(" (");
    
    char size_buf[16];
    for (int i = 0; i < 16; i++) size_buf[i] = 0;
    int size_len = 0;
    size_t temp_size = size;
    do {
        size_buf[size_len++] = '0' + (temp_size % 10);
        temp_size /= 10;
    } while (temp_size > 0 && size_len < 15);
    for (int i = size_len - 1; i >= 0; i--) {
        console_putc(size_buf[i]);
    }
    
    console_write(" bytes)\n");
    
    if (size == 0)
    {
        console_write("exec: empty file\n");
        return -1;
    }
    
    if (size < 4)
    {
        console_write("exec: file too small\n");
        return -1;
    }
    
    uint8_t* code_start = BINARY_LOAD_ADDR;
    
    if (BINARY_LOAD_ADDR[0] == 0x7F && BINARY_LOAD_ADDR[1] == 'E' && 
        BINARY_LOAD_ADDR[2] == 'L' && BINARY_LOAD_ADDR[3] == 'F')
    {
        console_write("exec: ELF format not supported (use flat binary)\n");
        return -1;
    }
    
    // Copy the appropriate stub into the binary buffer at offset 0x1000
    // This places it in user-space memory where the binary can access it
    uint8_t* stub_location = BINARY_LOAD_ADDR + 0x1000;
    
#if defined(__x86_64__) || defined(__amd64__)
    // Copy 64-bit stub
    for (int i = 0; i < sizeof(syscall_handler_stub); i++)
    {
        stub_location[i] = syscall_handler_stub[i];
    }
    
    uint64_t handler_addr = (uint64_t)syscall_handler_c;
    for (int i = 0; i < 8; i++)
    {
        stub_location[18 + i] = (uint8_t)(handler_addr >> (i * 8));
    }
    
    // Write stub address to syscall_entry (at offset 0)
    *(uint64_t*)&BINARY_LOAD_ADDR[0] = (uint64_t)(BINARY_LOAD_ADDR + 0x1000);
    
    console_write("exec: 64-bit stub at 0x");
    for (int i = 7; i >= 0; i--)
    {
        uint8_t nibble = ((uint64_t)(BINARY_LOAD_ADDR + 0x1000) >> (i * 4)) & 0x0F;
        console_putc(nibble < 10 ? ('0' + nibble) : ('a' + (nibble - 10)));
    }
    console_write("\n");
#else
    // Copy 32-bit stub
    for (int i = 0; i < sizeof(syscall_handler_stub); i++)
    {
        stub_location[i] = syscall_handler_stub[i];
    }
    
    uint32_t handler_addr = (uint32_t)syscall_handler_c;
    for (int i = 0; i < 4; i++)
    {
        stub_location[9 + i] = (uint8_t)(handler_addr >> (i * 8));
    }
    
    // Write stub address to syscall_entry (at offset 0)
    *(uint32_t*)&BINARY_LOAD_ADDR[0] = (uint32_t)(BINARY_LOAD_ADDR + 0x1000);
#endif
    
    binary_entry_t entry = (binary_entry_t)(code_start + sizeof(void*));
    console_write("exec: jumping to 0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (((size_t)entry) >> (i * 4)) & 0x0F;
        console_putc(nibble < 10 ? ('0' + nibble) : ('a' + (nibble - 10)));
    }
    console_write("\n");
    
    entry();
    console_write("exec: returned from binary\n");
    
    return 0;
}
