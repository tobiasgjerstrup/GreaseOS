#include "fat.h"
#include "console.h"
#include "drivers/ata.h"

#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_LFN 0x0F

#define FAT_EOC_16 0xFFF8

struct fat_fs
{
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t sectors_per_fat;
    uint32_t total_sectors;
    uint32_t root_dir_lba;
    uint32_t root_dir_sectors;
    uint32_t data_lba;
    uint16_t current_dir_cluster;
    uint32_t base_lba;
};

static struct fat_fs g_fs;
static char g_cwd[128] = "/";
static const char* g_error = "";

static uint16_t le16(const uint8_t* p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void mem_set(uint8_t* dst, uint8_t value, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        dst[i] = value;
    }
}

static void mem_copy(uint8_t* dst, const uint8_t* src, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
}

static size_t str_len(const char* s)
{
    size_t len = 0;
    while (s[len] != '\0')
    {
        len++;
    }
    return len;
}

static void set_error(const char* msg)
{
    g_error = msg;
}

const char* fat_last_error(void)
{
    return g_error;
}

static void uint32_to_str(uint32_t num, char* buf, size_t size)
{
    if (size < 2) return;
    if (num == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[32];
    int len = 0;
    uint32_t n = num;
    while (n > 0)
    {
        temp[len++] = '0' + (n % 10);
        n /= 10;
    }
    int idx = 0;
    for (int i = len - 1; i >= 0 && idx + 1 < (int)size; i--)
    {
        buf[idx++] = temp[i];
    }
    buf[idx] = '\0';
}

static int fat_make_name(const char* input, char out[11])
{
    for (int i = 0; i < 11; ++i)
    {
        out[i] = ' ';
    }

    if (input[0] == '.' && input[1] == '\0')
    {
        out[0] = '.';
        return 0;
    }
    if (input[0] == '.' && input[1] == '.' && input[2] == '\0')
    {
        out[0] = '.';
        out[1] = '.';
        return 0;
    }

    int i = 0;
    int out_index = 0;
    while (input[i] != '\0' && input[i] != '.' && out_index < 8)
    {
        char c = input[i];
        if (c >= 'a' && c <= 'z')
        {
            c = (char)(c - 'a' + 'A');
        }
        out[out_index++] = c;
        i++;
    }

    if (out_index == 0)
    {
        return -1;
    }

    if (input[i] == '.')
    {
        i++;
        int ext_index = 8;
        while (input[i] != '\0' && ext_index < 11)
        {
            char c = input[i];
            if (c >= 'a' && c <= 'z')
            {
                c = (char)(c - 'a' + 'A');
            }
            out[ext_index++] = c;
            i++;
        }
    }

    if (input[i] != '\0')
    {
        return -1;
    }

    return 0;
}

static void fat_format_name(const uint8_t* entry, char* out, size_t out_len)
{
    size_t pos = 0;
    for (int i = 0; i < 8 && entry[i] != ' '; ++i)
    {
        if (pos + 1 < out_len)
        {
            out[pos++] = (char)entry[i];
        }
    }

    int has_ext = 0;
    for (int i = 8; i < 11; ++i)
    {
        if (entry[i] != ' ')
        {
            has_ext = 1;
            break;
        }
    }

    if (has_ext && pos + 1 < out_len)
    {
        out[pos++] = '.';
        for (int i = 8; i < 11 && entry[i] != ' '; ++i)
        {
            if (pos + 1 < out_len)
            {
                out[pos++] = (char)entry[i];
            }
        }
    }

    if (pos < out_len)
    {
        out[pos] = '\0';
    }
}

static uint32_t fat_cluster_to_lba(uint16_t cluster)
{
    return g_fs.data_lba + ((uint32_t)(cluster - 2) * g_fs.sectors_per_cluster);
}

static int fat_read_sector(uint32_t lba, uint8_t* buffer)
{
    int rc = ata_read_sector(g_fs.base_lba + lba, buffer);
    if (rc == -2)
    {
        set_error("No ATA device");
        return -1;
    }
    if (rc != 0)
    {
        set_error("Disk read failed");
        return -1;
    }
    return 0;
}

static int fat_write_sector(uint32_t lba, const uint8_t* buffer)
{
    int rc = ata_write_sector(g_fs.base_lba + lba, buffer);
    if (rc == -2)
    {
        set_error("No ATA device");
        return -1;
    }
    if (rc != 0)
    {
        set_error("Disk write failed");
        return -1;
    }
    return 0;
}

static int fat_bpb_valid(const uint8_t* sector)
{
    uint16_t bytes_per_sector = le16(&sector[11]);
    uint8_t sectors_per_cluster = sector[13];
    uint8_t num_fats = sector[16];
    uint16_t root_entries = le16(&sector[17]);
    uint16_t sectors_per_fat = le16(&sector[22]);
    if (bytes_per_sector != 512 || sectors_per_cluster == 0 || sectors_per_fat == 0)
    {
        return 0;
    }
    if (num_fats == 0 || num_fats > 2 || root_entries == 0)
    {
        return 0;
    }
    return 1;
}

static int fat_read_entry(uint16_t cluster, uint16_t* value, uint8_t* sector)
{
    uint32_t fat_offset = (uint32_t)cluster * 2;
    uint32_t fat_sector = g_fs.reserved_sectors + (fat_offset / g_fs.bytes_per_sector);
    uint32_t fat_index = fat_offset % g_fs.bytes_per_sector;

    if (fat_read_sector(fat_sector, sector) != 0)
    {
        return -1;
    }

    *value = (uint16_t)sector[fat_index] | ((uint16_t)sector[fat_index + 1] << 8);
    return 0;
}

static int fat_write_entry(uint16_t cluster, uint16_t value, uint8_t* sector)
{
    uint32_t fat_offset = (uint32_t)cluster * 2;
    uint32_t fat_sector_index = fat_offset / g_fs.bytes_per_sector;
    uint32_t fat_index = fat_offset % g_fs.bytes_per_sector;

    for (uint8_t fat = 0; fat < g_fs.num_fats; ++fat)
    {
        uint32_t fat_sector = g_fs.reserved_sectors + fat_sector_index + (uint32_t)fat * g_fs.sectors_per_fat;
        if (fat_read_sector(fat_sector, sector) != 0)
        {
            return -1;
        }
        sector[fat_index] = (uint8_t)(value & 0xFF);
        sector[fat_index + 1] = (uint8_t)(value >> 8);
        if (fat_write_sector(fat_sector, sector) != 0)
        {
            return -1;
        }
    }
    return 0;
}

static int fat_find_free_cluster(uint16_t* out_cluster, uint8_t* sector)
{
    uint32_t entries_per_sector = g_fs.bytes_per_sector / 2;
    uint32_t total_entries = (uint32_t)g_fs.sectors_per_fat * entries_per_sector;

    for (uint32_t entry = 2; entry < total_entries; ++entry)
    {
        uint32_t fat_offset = entry * 2;
        uint32_t fat_sector = g_fs.reserved_sectors + (fat_offset / g_fs.bytes_per_sector);
        uint32_t fat_index = fat_offset % g_fs.bytes_per_sector;

        if (fat_read_sector(fat_sector, sector) != 0)
        {
            return -1;
        }

        uint16_t value = (uint16_t)sector[fat_index] | ((uint16_t)sector[fat_index + 1] << 8);
        if (value == 0x0000)
        {
            *out_cluster = (uint16_t)entry;
            return 0;
        }
    }

    set_error("No free clusters");
    return -1;
}

static int fat_is_eoc(uint16_t value)
{
    return value >= FAT_EOC_16;
}

static int fat_read_dir_sector(uint32_t lba, uint8_t* sector)
{
    return fat_read_sector(lba, sector);
}

static int fat_find_entry_in_dir(uint16_t dir_cluster, const char* name, uint8_t* entry_out, uint32_t* entry_lba, uint32_t* entry_offset)
{
    uint8_t sector[512];
    char target[11];
    if (fat_make_name(name, target) != 0)
    {
        set_error("Invalid name");
        return -1;
    }

    if (dir_cluster == 0)
    {
        for (uint32_t s = 0; s < g_fs.root_dir_sectors; ++s)
        {
            uint32_t lba = g_fs.root_dir_lba + s;
            if (fat_read_dir_sector(lba, sector) != 0)
            {
                return -1;
            }

            for (uint32_t i = 0; i < g_fs.bytes_per_sector; i += 32)
            {
                uint8_t first = sector[i];
                if (first == 0x00)
                {
                    return -1;
                }
                if (first == 0xE5)
                {
                    continue;
                }
                uint8_t attr = sector[i + 11];
                if (attr == FAT_ATTR_LFN || attr == FAT_ATTR_VOLUME_ID)
                {
                    continue;
                }

                int match = 1;
                for (int j = 0; j < 11; ++j)
                {
                    if (sector[i + j] != (uint8_t)target[j])
                    {
                        match = 0;
                        break;
                    }
                }
                if (match)
                {
                    mem_copy(entry_out, &sector[i], 32);
                    *entry_lba = lba;
                    *entry_offset = i;
                    return 0;
                }
            }
        }
        return -1;
    }

    uint16_t cluster = dir_cluster;
    while (cluster >= 2 && !fat_is_eoc(cluster))
    {
        uint32_t base = fat_cluster_to_lba(cluster);
        for (uint8_t s = 0; s < g_fs.sectors_per_cluster; ++s)
        {
            uint32_t lba = base + s;
            if (fat_read_dir_sector(lba, sector) != 0)
            {
                return -1;
            }
            for (uint32_t i = 0; i < g_fs.bytes_per_sector; i += 32)
            {
                uint8_t first = sector[i];
                if (first == 0x00)
                {
                    return -1;
                }
                if (first == 0xE5)
                {
                    continue;
                }
                uint8_t attr = sector[i + 11];
                if (attr == FAT_ATTR_LFN || attr == FAT_ATTR_VOLUME_ID)
                {
                    continue;
                }

                int match = 1;
                for (int j = 0; j < 11; ++j)
                {
                    if (sector[i + j] != (uint8_t)target[j])
                    {
                        match = 0;
                        break;
                    }
                }
                if (match)
                {
                    mem_copy(entry_out, &sector[i], 32);
                    *entry_lba = lba;
                    *entry_offset = i;
                    return 0;
                }
            }
        }

        uint8_t fat_sector[512];
        uint16_t next = 0;
        if (fat_read_entry(cluster, &next, fat_sector) != 0)
        {
            return -1;
        }
        cluster = next;
    }

    return -1;
}

static int fat_find_free_dir_entry(uint16_t dir_cluster, uint32_t* entry_lba, uint32_t* entry_offset, uint8_t* sector)
{
    if (dir_cluster == 0)
    {
        for (uint32_t s = 0; s < g_fs.root_dir_sectors; ++s)
        {
            uint32_t lba = g_fs.root_dir_lba + s;
            if (fat_read_dir_sector(lba, sector) != 0)
            {
                return -1;
            }
            for (uint32_t i = 0; i < g_fs.bytes_per_sector; i += 32)
            {
                if (sector[i] == 0x00 || sector[i] == 0xE5)
                {
                    *entry_lba = lba;
                    *entry_offset = i;
                    return 0;
                }
            }
        }
        set_error("Root directory full");
        return -1;
    }

    uint16_t cluster = dir_cluster;
    while (cluster >= 2 && !fat_is_eoc(cluster))
    {
        uint32_t base = fat_cluster_to_lba(cluster);
        for (uint8_t s = 0; s < g_fs.sectors_per_cluster; ++s)
        {
            uint32_t lba = base + s;
            if (fat_read_dir_sector(lba, sector) != 0)
            {
                return -1;
            }
            for (uint32_t i = 0; i < g_fs.bytes_per_sector; i += 32)
            {
                if (sector[i] == 0x00 || sector[i] == 0xE5)
                {
                    *entry_lba = lba;
                    *entry_offset = i;
                    return 0;
                }
            }
        }

        uint8_t fat_sector[512];
        uint16_t next = 0;
        if (fat_read_entry(cluster, &next, fat_sector) != 0)
        {
            return -1;
        }

        if (fat_is_eoc(next))
        {
            uint16_t new_cluster = 0;
            if (fat_find_free_cluster(&new_cluster, fat_sector) != 0)
            {
                return -1;
            }
            if (fat_write_entry(cluster, new_cluster, fat_sector) != 0)
            {
                return -1;
            }
            if (fat_write_entry(new_cluster, 0xFFFF, fat_sector) != 0)
            {
                return -1;
            }

            uint8_t zero[512];
            mem_set(zero, 0, sizeof(zero));
            uint32_t base_new = fat_cluster_to_lba(new_cluster);
            for (uint8_t s = 0; s < g_fs.sectors_per_cluster; ++s)
            {
                if (fat_write_sector(base_new + s, zero) != 0)
                {
                    return -1;
                }
            }

            if (fat_read_dir_sector(base_new, sector) != 0)
            {
                return -1;
            }
            *entry_lba = base_new;
            *entry_offset = 0;
            return 0;
        }

        cluster = next;
    }

    set_error("Directory full");
    return -1;
}

static void fat_write_dir_entry(uint8_t* entry, const char name[11], uint8_t attr, uint16_t cluster, uint32_t size)
{
    for (int i = 0; i < 11; ++i)
    {
        entry[i] = (uint8_t)name[i];
    }
    entry[11] = attr;
    for (int i = 12; i < 26; ++i)
    {
        entry[i] = 0;
    }
    entry[26] = (uint8_t)(cluster & 0xFF);
    entry[27] = (uint8_t)(cluster >> 8);
    entry[28] = (uint8_t)(size & 0xFF);
    entry[29] = (uint8_t)((size >> 8) & 0xFF);
    entry[30] = (uint8_t)((size >> 16) & 0xFF);
    entry[31] = (uint8_t)((size >> 24) & 0xFF);
}

static int fat_free_chain(uint16_t cluster)
{
    if (cluster < 2)
    {
        return 0;
    }

    uint8_t sector[512];
    while (cluster >= 2)
    {
        uint16_t next = 0;
        if (fat_read_entry(cluster, &next, sector) != 0)
        {
            return -1;
        }
        if (fat_write_entry(cluster, 0x0000, sector) != 0)
        {
            return -1;
        }
        if (fat_is_eoc(next))
        {
            break;
        }
        cluster = next;
    }

    return 0;
}

int fat_init(void)
{
    uint8_t sector[512];
    g_fs.base_lba = 0;
    if (fat_read_sector(0, sector) != 0)
    {
        return -1;
    }

    if (!fat_bpb_valid(sector))
    {
        if (sector[510] != 0x55 || sector[511] != 0xAA)
        {
            set_error("No boot sector");
            return -1;
        }

        uint32_t part_lba = 0;
        for (int i = 0; i < 4; ++i)
        {
            uint32_t entry = 446 + (uint32_t)i * 16;
            uint8_t type = sector[entry + 4];
            if (type == 0)
            {
                continue;
            }
            part_lba = le32(&sector[entry + 8]);
            break;
        }

        if (part_lba == 0)
        {
            set_error("No FAT partition");
            return -1;
        }

        g_fs.base_lba = part_lba;
        if (fat_read_sector(0, sector) != 0)
        {
            return -1;
        }
        if (!fat_bpb_valid(sector))
        {
            set_error("Unsupported FAT format");
            return -1;
        }
    }

    g_fs.bytes_per_sector = le16(&sector[11]);
    g_fs.sectors_per_cluster = sector[13];
    g_fs.reserved_sectors = le16(&sector[14]);
    g_fs.num_fats = sector[16];
    g_fs.root_entries = le16(&sector[17]);
    uint16_t total16 = le16(&sector[19]);
    g_fs.sectors_per_fat = le16(&sector[22]);
    uint32_t total32 = le32(&sector[32]);
    g_fs.total_sectors = total16 != 0 ? total16 : total32;

    if (g_fs.bytes_per_sector != 512 || g_fs.sectors_per_fat == 0)
    {
        set_error("Unsupported FAT format");
        return -1;
    }

    g_fs.root_dir_sectors = (uint32_t)((g_fs.root_entries * 32 + g_fs.bytes_per_sector - 1) / g_fs.bytes_per_sector);
    g_fs.root_dir_lba = g_fs.reserved_sectors + (uint32_t)g_fs.num_fats * g_fs.sectors_per_fat;
    g_fs.data_lba = g_fs.root_dir_lba + g_fs.root_dir_sectors;

    uint32_t data_sectors = g_fs.total_sectors - (g_fs.reserved_sectors + (uint32_t)g_fs.num_fats * g_fs.sectors_per_fat + g_fs.root_dir_sectors);
    uint32_t cluster_count = data_sectors / g_fs.sectors_per_cluster;
    if (cluster_count < 4085)
    {
        set_error("FAT12 not supported");
        return -1;
    }
    if (cluster_count >= 65525)
    {
        set_error("FAT32 not supported");
        return -1;
    }

    g_fs.current_dir_cluster = 0;
    g_cwd[0] = '/';
    g_cwd[1] = '\0';
    set_error("");
    return 0;
}

int fat_ls(void)
{
    uint8_t sector[512];
    char name[16];

    if (g_fs.current_dir_cluster == 0)
    {
        for (uint32_t s = 0; s < g_fs.root_dir_sectors; ++s)
        {
            uint32_t lba = g_fs.root_dir_lba + s;
            if (fat_read_dir_sector(lba, sector) != 0)
            {
                return -1;
            }
            for (uint32_t i = 0; i < g_fs.bytes_per_sector; i += 32)
            {
                uint8_t first = sector[i];
                if (first == 0x00)
                {
                    return 0;
                }
                if (first == 0xE5)
                {
                    continue;
                }
                uint8_t attr = sector[i + 11];
                if (attr == FAT_ATTR_LFN || attr == FAT_ATTR_VOLUME_ID)
                {
                    continue;
                }
                fat_format_name(&sector[i], name, sizeof(name));
                if (attr & FAT_ATTR_DIRECTORY)
                {
                    console_write("<DIR> ");
                    console_write(name);
                    console_putc('\n');
                }
                else
                {
                    console_write("      ");
                    console_write(name);
                    console_putc('\n');
                }
            }
        }
        return 0;
    }

    uint16_t cluster = g_fs.current_dir_cluster;
    while (cluster >= 2 && !fat_is_eoc(cluster))
    {
        uint32_t base = fat_cluster_to_lba(cluster);
        for (uint8_t s = 0; s < g_fs.sectors_per_cluster; ++s)
        {
            if (fat_read_dir_sector(base + s, sector) != 0)
            {
                return -1;
            }
            for (uint32_t i = 0; i < g_fs.bytes_per_sector; i += 32)
            {
                uint8_t first = sector[i];
                if (first == 0x00)
                {
                    return 0;
                }
                if (first == 0xE5)
                {
                    continue;
                }
                uint8_t attr = sector[i + 11];
                if (attr == FAT_ATTR_LFN || attr == FAT_ATTR_VOLUME_ID)
                {
                    continue;
                }
                fat_format_name(&sector[i], name, sizeof(name));
                if (attr & FAT_ATTR_DIRECTORY)
                {
                    console_write("<DIR> ");
                    console_write(name);
                    console_putc('\n');
                }
                else
                {
                    console_write("      ");
                    console_write(name);
                    console_putc('\n');
                }
            }
        }
        uint8_t fat_sector[512];
        uint16_t next = 0;
        if (fat_read_entry(cluster, &next, fat_sector) != 0)
        {
            return -1;
        }
        cluster = next;
    }

    return 0;
}

int fat_cd(const char* name)
{
    if (name == 0 || name[0] == '\0')
    {
        return 0;
    }

    if (name[0] == '/' && name[1] == '\0')
    {
        g_fs.current_dir_cluster = 0;
        g_cwd[0] = '/';
        g_cwd[1] = '\0';
        return 0;
    }

    if (name[0] == '.' && name[1] == '\0')
    {
        return 0;
    }

    if (name[0] == '.' && name[1] == '.' && name[2] == '\0' && g_fs.current_dir_cluster == 0)
    {
        return 0;
    }

    uint8_t entry[32];
    uint32_t lba = 0;
    uint32_t offset = 0;
    if (fat_find_entry_in_dir(g_fs.current_dir_cluster, name, entry, &lba, &offset) != 0)
    {
        set_error("Not found");
        return -1;
    }

    if ((entry[11] & FAT_ATTR_DIRECTORY) == 0)
    {
        set_error("Not a directory");
        return -1;
    }

    uint16_t cluster = (uint16_t)entry[26] | ((uint16_t)entry[27] << 8);
    if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
    {
        g_fs.current_dir_cluster = cluster;
        size_t len = str_len(g_cwd);
        if (len > 1)
        {
            size_t i = len - 1;
            while (i > 0 && g_cwd[i] != '/')
            {
                i--;
            }
            if (i == 0)
            {
                g_cwd[0] = '/';
                g_cwd[1] = '\0';
            }
            else
            {
                g_cwd[i] = '\0';
            }
        }
        return 0;
    }

    g_fs.current_dir_cluster = cluster;
    size_t len = str_len(g_cwd);
    if (len == 1 && g_cwd[0] == '/')
    {
        size_t i = 0;
        while (name[i] != '\0' && i + 2 < sizeof(g_cwd))
        {
            g_cwd[1 + i] = name[i];
            g_cwd[2 + i] = '\0';
            i++;
        }
    }
    else
    {
        if (len + 1 < sizeof(g_cwd))
        {
            g_cwd[len++] = '/';
            g_cwd[len] = '\0';
        }
        size_t i = 0;
        while (name[i] != '\0' && len + i + 1 < sizeof(g_cwd))
        {
            g_cwd[len + i] = name[i];
            g_cwd[len + i + 1] = '\0';
            i++;
        }
    }

    return 0;
}

const char* fat_pwd(void)
{
    return g_cwd;
}

int fat_mkdir(const char* name)
{
    if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
    {
        set_error("Invalid name");
        return -1;
    }
    uint8_t entry[32];
    uint32_t lba = 0;
    uint32_t offset = 0;
    if (fat_find_entry_in_dir(g_fs.current_dir_cluster, name, entry, &lba, &offset) == 0)
    {
        set_error("Already exists");
        return -1;
    }

    uint8_t sector[512];
    if (fat_find_free_dir_entry(g_fs.current_dir_cluster, &lba, &offset, sector) != 0)
    {
        return -1;
    }

    uint16_t new_cluster = 0;
    uint8_t fat_sector[512];
    if (fat_find_free_cluster(&new_cluster, fat_sector) != 0)
    {
        return -1;
    }
    if (fat_write_entry(new_cluster, 0xFFFF, fat_sector) != 0)
    {
        return -1;
    }

    char fat_name[11];
    if (fat_make_name(name, fat_name) != 0)
    {
        set_error("Invalid name");
        return -1;
    }

    fat_write_dir_entry(&sector[offset], fat_name, FAT_ATTR_DIRECTORY, new_cluster, 0);
    if (fat_write_sector(lba, sector) != 0)
    {
        return -1;
    }

    uint8_t dir_sector[512];
    mem_set(dir_sector, 0, sizeof(dir_sector));

    char dot[11];
    mem_set((uint8_t*)dot, ' ', 11);
    dot[0] = '.';
    fat_write_dir_entry(dir_sector, dot, FAT_ATTR_DIRECTORY, new_cluster, 0);

    char dotdot[11];
    mem_set((uint8_t*)dotdot, ' ', 11);
    dotdot[0] = '.';
    dotdot[1] = '.';
    fat_write_dir_entry(&dir_sector[32], dotdot, FAT_ATTR_DIRECTORY, g_fs.current_dir_cluster, 0);

    uint32_t base = fat_cluster_to_lba(new_cluster);
    if (fat_write_sector(base, dir_sector) != 0)
    {
        return -1;
    }

    for (uint8_t s = 1; s < g_fs.sectors_per_cluster; ++s)
    {
        uint8_t zero[512];
        mem_set(zero, 0, sizeof(zero));
        if (fat_write_sector(base + s, zero) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int fat_touch(const char* name)
{
    if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
    {
        set_error("Invalid name");
        return -1;
    }
    uint8_t entry[32];
    uint32_t lba = 0;
    uint32_t offset = 0;
    if (fat_find_entry_in_dir(g_fs.current_dir_cluster, name, entry, &lba, &offset) == 0)
    {
        set_error("Already exists");
        return -1;
    }

    uint8_t sector[512];
    if (fat_find_free_dir_entry(g_fs.current_dir_cluster, &lba, &offset, sector) != 0)
    {
        return -1;
    }

    char fat_name[11];
    if (fat_make_name(name, fat_name) != 0)
    {
        set_error("Invalid name");
        return -1;
    }

    fat_write_dir_entry(&sector[offset], fat_name, 0x20, 0, 0);
    if (fat_write_sector(lba, sector) != 0)
    {
        return -1;
    }

    return 0;
}

int fat_cat(const char* name)
{
    uint8_t entry[32];
    uint32_t lba = 0;
    uint32_t offset = 0;
    if (fat_find_entry_in_dir(g_fs.current_dir_cluster, name, entry, &lba, &offset) != 0)
    {
        set_error("Not found");
        return -1;
    }

    if (entry[11] & FAT_ATTR_DIRECTORY)
    {
        set_error("Is a directory");
        return -1;
    }

    uint32_t size = le32(&entry[28]);
    uint16_t cluster = (uint16_t)entry[26] | ((uint16_t)entry[27] << 8);

    if (size == 0 || cluster == 0)
    {
        console_putc('\n');
        return 0;
    }

    uint8_t sector[512];
    uint32_t remaining = size;

    while (cluster >= 2 && !fat_is_eoc(cluster) && remaining > 0)
    {
        uint32_t base = fat_cluster_to_lba(cluster);
        for (uint8_t s = 0; s < g_fs.sectors_per_cluster && remaining > 0; ++s)
        {
            if (fat_read_sector(base + s, sector) != 0)
            {
                return -1;
            }
            for (uint32_t i = 0; i < g_fs.bytes_per_sector && remaining > 0; ++i)
            {
                console_putc((char)sector[i]);
                remaining--;
            }
        }

        uint8_t fat_sector[512];
        uint16_t next = 0;
        if (fat_read_entry(cluster, &next, fat_sector) != 0)
        {
            return -1;
        }
        cluster = next;
    }

    console_putc('\n');
    return 0;
}

int fat_read(const char* name, char* out, size_t max, size_t* out_size)
{
    if (out_size)
    {
        *out_size = 0;
    }

    uint8_t entry[32];
    uint32_t lba = 0;
    uint32_t offset = 0;
    if (fat_find_entry_in_dir(g_fs.current_dir_cluster, name, entry, &lba, &offset) != 0)
    {
        set_error("Not found");
        return -1;
    }

    if (entry[11] & FAT_ATTR_DIRECTORY)
    {
        set_error("Is a directory");
        return -1;
    }

    uint32_t size = le32(&entry[28]);
    uint16_t cluster = (uint16_t)entry[26] | ((uint16_t)entry[27] << 8);

    if (size == 0 || cluster == 0)
    {
        if (out && max > 0)
        {
            out[0] = '\0';
        }
        if (out_size)
        {
            *out_size = 0;
        }
        return 0;
    }

    if (size + 1 > max)
    {
        set_error("Buffer too small");
        return -1;
    }

    uint8_t sector[512];
    uint32_t remaining = size;
    size_t written = 0;

    while (cluster >= 2 && !fat_is_eoc(cluster) && remaining > 0)
    {
        uint32_t base = fat_cluster_to_lba(cluster);
        for (uint8_t s = 0; s < g_fs.sectors_per_cluster && remaining > 0; ++s)
        {
            if (fat_read_sector(base + s, sector) != 0)
            {
                return -1;
            }
            for (uint32_t i = 0; i < g_fs.bytes_per_sector && remaining > 0; ++i)
            {
                out[written++] = (char)sector[i];
                remaining--;
            }
        }

        uint8_t fat_sector[512];
        uint16_t next = 0;
        if (fat_read_entry(cluster, &next, fat_sector) != 0)
        {
            return -1;
        }
        cluster = next;
    }

    out[written] = '\0';
    if (out_size)
    {
        *out_size = written;
    }
    return 0;
}

int fat_df(void)
{
    uint32_t entries_per_sector = g_fs.bytes_per_sector / 2;
    uint32_t total_entries = (uint32_t)g_fs.sectors_per_fat * entries_per_sector;
    uint32_t free_clusters = 0;
    uint32_t used_clusters = 0;

    for (uint32_t entry = 2; entry < total_entries; ++entry)
    {
        uint32_t fat_offset = entry * 2;
        uint32_t fat_sector = g_fs.reserved_sectors + (fat_offset / g_fs.bytes_per_sector);
        uint32_t fat_index = fat_offset % g_fs.bytes_per_sector;

        uint8_t sector[512];
        if (fat_read_sector(fat_sector, sector) != 0)
        {
            return -1;
        }

        uint16_t value = (uint16_t)sector[fat_index] | ((uint16_t)sector[fat_index + 1] << 8);
        if (value == 0x0000)
        {
            free_clusters++;
        }
        else
        {
            used_clusters++;
        }
    }

    uint32_t cluster_size_kb = (uint32_t)g_fs.bytes_per_sector * g_fs.sectors_per_cluster / 1024;
    uint32_t total_size_kb = (free_clusters + used_clusters) * cluster_size_kb;
    uint32_t used_size_kb = used_clusters * cluster_size_kb;
    uint32_t free_size_kb = free_clusters * cluster_size_kb;

    char buf[32];
    console_write("Disk usage:\n");
    console_write("Total: ");
    uint32_to_str(total_size_kb, buf, sizeof(buf));
    console_write(buf);
    console_write(" KB\n");
    console_write("Used:  ");
    uint32_to_str(used_size_kb, buf, sizeof(buf));
    console_write(buf);
    console_write(" KB\n");
    console_write("Free:  ");
    uint32_to_str(free_size_kb, buf, sizeof(buf));
    console_write(buf);
    console_write(" KB\n");

    return 0;
}

int fat_write_data(const char* name, const char* data, size_t data_len)
{
    if (name == 0 || name[0] == '\0')
    {
        set_error("Invalid name");
        return -1;
    }
    if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
    {
        set_error("Invalid name");
        return -1;
    }

    uint8_t entry[32];
    uint32_t lba = 0;
    uint32_t offset = 0;
    int exists = (fat_find_entry_in_dir(g_fs.current_dir_cluster, name, entry, &lba, &offset) == 0);

    uint16_t cluster = 0;
    if (exists)
    {
        if (entry[11] & FAT_ATTR_DIRECTORY)
        {
            set_error("Is a directory");
            return -1;
        }
        cluster = (uint16_t)entry[26] | ((uint16_t)entry[27] << 8);
    }

    if (cluster != 0)
    {
        if (fat_free_chain(cluster) != 0)
        {
            return -1;
        }
        cluster = 0;
    }

    uint32_t cluster_size = (uint32_t)g_fs.bytes_per_sector * g_fs.sectors_per_cluster;
    uint32_t clusters_needed = 0;
    if (data_len > 0)
    {
        clusters_needed = (data_len + cluster_size - 1) / cluster_size;
    }

    uint16_t first_cluster = 0;
    uint16_t prev_cluster = 0;
    for (uint32_t i = 0; i < clusters_needed; ++i)
    {
        uint8_t fat_sector[512];
        uint16_t new_cluster = 0;
        if (fat_find_free_cluster(&new_cluster, fat_sector) != 0)
        {
            if (first_cluster != 0)
            {
                fat_free_chain(first_cluster);
            }
            return -1;
        }
        if (prev_cluster != 0)
        {
            if (fat_write_entry(prev_cluster, new_cluster, fat_sector) != 0)
            {
                fat_free_chain(first_cluster);
                return -1;
            }
        }
        prev_cluster = new_cluster;
        if (first_cluster == 0)
        {
            first_cluster = new_cluster;
        }
    }

    if (prev_cluster != 0)
    {
        uint8_t fat_sector[512];
        if (fat_write_entry(prev_cluster, 0xFFFF, fat_sector) != 0)
        {
            fat_free_chain(first_cluster);
            return -1;
        }
    }

    cluster = first_cluster;

    if (!exists)
    {
        uint8_t sector[512];
        if (fat_find_free_dir_entry(g_fs.current_dir_cluster, &lba, &offset, sector) != 0)
        {
            return -1;
        }

        char fat_name[11];
        if (fat_make_name(name, fat_name) != 0)
        {
            set_error("Invalid name");
            return -1;
        }

        fat_write_dir_entry(&sector[offset], fat_name, 0x20, cluster, (uint32_t)data_len);
        if (fat_write_sector(lba, sector) != 0)
        {
            return -1;
        }
    }
    else
    {
        uint8_t sector[512];
        if (fat_read_sector(lba, sector) != 0)
        {
            return -1;
        }
        sector[offset + 26] = (uint8_t)(cluster & 0xFF);
        sector[offset + 27] = (uint8_t)(cluster >> 8);
        sector[offset + 28] = (uint8_t)(data_len & 0xFF);
        sector[offset + 29] = (uint8_t)((data_len >> 8) & 0xFF);
        sector[offset + 30] = (uint8_t)((data_len >> 16) & 0xFF);
        sector[offset + 31] = (uint8_t)((data_len >> 24) & 0xFF);
        if (fat_write_sector(lba, sector) != 0)
        {
            return -1;
        }
    }

    if (data_len == 0)
    {
        return 0;
    }

    size_t written = 0;
    while (cluster >= 2 && !fat_is_eoc(cluster) && written < data_len)
    {
        uint32_t base = fat_cluster_to_lba(cluster);
        for (uint8_t s = 0; s < g_fs.sectors_per_cluster; ++s)
        {
            uint8_t sector[512];
            mem_set(sector, 0, sizeof(sector));
            for (uint32_t i = 0; i < g_fs.bytes_per_sector && written < data_len; ++i)
            {
                sector[i] = (uint8_t)data[written++];
            }
            if (fat_write_sector(base + s, sector) != 0)
            {
                return -1;
            }
        }

        if (written >= data_len)
        {
            break;
        }

        uint8_t fat_sector[512];
        uint16_t next = 0;
        if (fat_read_entry(cluster, &next, fat_sector) != 0)
        {
            return -1;
        }
        cluster = next;
    }

    return 0;
}

int fat_write(const char* name, const char* data)
{
    return fat_write_data(name, data, str_len(data));
}
