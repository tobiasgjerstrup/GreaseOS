#include "ata.h"
#include "io.h"

#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECCOUNT0  0x1F2
#define ATA_LBA0       0x1F3
#define ATA_LBA1       0x1F4
#define ATA_LBA2       0x1F5
#define ATA_HDDEVSEL   0x1F6
#define ATA_COMMAND    0x1F7
#define ATA_STATUS     0x1F7

#define ATA_CMD_READ   0x20
#define ATA_CMD_WRITE  0x30
#define ATA_CMD_FLUSH  0xE7

#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

static int ata_wait_busy(void)
{
    for (uint32_t i = 0; i < 100000; ++i)
    {
        if ((inb(ATA_STATUS) & ATA_SR_BSY) == 0)
        {
            return 0;
        }
    }
    return -1;
}

static int ata_wait_drq(void)
{
    for (uint32_t i = 0; i < 100000; ++i)
    {
        uint8_t status = inb(ATA_STATUS);
        if (status & ATA_SR_ERR)
        {
            return -1;
        }
        if (status & ATA_SR_DRQ)
        {
            return 0;
        }
    }
    return -1;
}

static void ata_select_drive(uint32_t lba)
{
    outb(ATA_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
}

int ata_read_sector(uint32_t lba, uint8_t* buffer)
{
    if (ata_wait_busy() != 0)
    {
        return -1;
    }

    ata_select_drive(lba);
    outb(ATA_SECCOUNT0, 1);
    outb(ATA_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (ata_wait_busy() != 0 || ata_wait_drq() != 0)
    {
        return -1;
    }

    for (uint32_t i = 0; i < 256; ++i)
    {
        uint16_t data = inw(ATA_DATA);
        buffer[i * 2] = (uint8_t)(data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }

    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer)
{
    if (ata_wait_busy() != 0)
    {
        return -1;
    }

    ata_select_drive(lba);
    outb(ATA_SECCOUNT0, 1);
    outb(ATA_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (ata_wait_busy() != 0 || ata_wait_drq() != 0)
    {
        return -1;
    }

    for (uint32_t i = 0; i < 256; ++i)
    {
        uint16_t data = (uint16_t)buffer[i * 2] | ((uint16_t)buffer[i * 2 + 1] << 8);
        outw(ATA_DATA, data);
    }

    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    if (ata_wait_busy() != 0)
    {
        return -1;
    }

    return 0;
}
