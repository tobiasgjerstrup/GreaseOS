#include "framebuffer.h"

#include <stddef.h>

struct mb2_tag
{
    uint32_t type;
    uint32_t size;
};

struct mb2_tag_framebuffer
{
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
    uint8_t red_field_position;
    uint8_t red_mask_size;
    uint8_t green_field_position;
    uint8_t green_mask_size;
    uint8_t blue_field_position;
    uint8_t blue_mask_size;
} __attribute__((packed));

static struct fb_info g_fb;
static int g_fb_ready = 0;

static uint32_t pack_rgb(uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    uint8_t r_size = g_fb.red_size > 8 ? 8 : g_fb.red_size;
    uint8_t g_size = g_fb.green_size > 8 ? 8 : g_fb.green_size;
    uint8_t b_size = g_fb.blue_size > 8 ? 8 : g_fb.blue_size;

    uint32_t out = 0;
    if (r_size)
    {
        uint32_t val = r >> (8 - r_size);
        out |= (val & ((1u << r_size) - 1u)) << g_fb.red_pos;
    }
    if (g_size)
    {
        uint32_t val = g >> (8 - g_size);
        out |= (val & ((1u << g_size) - 1u)) << g_fb.green_pos;
    }
    if (b_size)
    {
        uint32_t val = b >> (8 - b_size);
        out |= (val & ((1u << b_size) - 1u)) << g_fb.blue_pos;
    }
    return out;
}

int fb_init(const void *mb2_info)
{
    g_fb_ready = 0;
    if (!mb2_info)
    {
        return -1;
    }

    const uint8_t *start = (const uint8_t *)mb2_info;
    uint32_t total_size = *(const uint32_t *)start;
    if (total_size < 8)
    {
        return -1;
    }

    const uint8_t *end = start + total_size;
    const uint8_t *ptr = start + 8;

    while (ptr + sizeof(struct mb2_tag) <= end)
    {
        const struct mb2_tag *tag = (const struct mb2_tag *)ptr;
        if (tag->type == 0)
        {
            break;
        }

        if (tag->type == 8 && tag->size >= sizeof(struct mb2_tag_framebuffer))
        {
            const struct mb2_tag_framebuffer *fb = (const struct mb2_tag_framebuffer *)ptr;
            if (fb->framebuffer_type != 1)
            {
                return -1;
            }

            g_fb.address = fb->framebuffer_addr;
            g_fb.pitch = fb->framebuffer_pitch;
            g_fb.width = fb->framebuffer_width;
            g_fb.height = fb->framebuffer_height;
            g_fb.bpp = fb->framebuffer_bpp;
            g_fb.red_pos = fb->red_field_position;
            g_fb.red_size = fb->red_mask_size;
            g_fb.green_pos = fb->green_field_position;
            g_fb.green_size = fb->green_mask_size;
            g_fb.blue_pos = fb->blue_field_position;
            g_fb.blue_size = fb->blue_mask_size;

            if (g_fb.red_size == 0 && g_fb.green_size == 0 && g_fb.blue_size == 0)
            {
                g_fb.red_pos = 16;
                g_fb.red_size = 8;
                g_fb.green_pos = 8;
                g_fb.green_size = 8;
                g_fb.blue_pos = 0;
                g_fb.blue_size = 8;
            }

            if (g_fb.bpp != 32 && g_fb.bpp != 24)
            {
                return -1;
            }

            g_fb_ready = 1;
            return 0;
        }

        uint32_t size = tag->size;
        if (size == 0)
        {
            break;
        }
        ptr += (size + 7u) & ~7u;
    }

    return -1;
}

int fb_is_available(void)
{
    return g_fb_ready;
}

const struct fb_info *fb_get_info(void)
{
    if (!g_fb_ready)
    {
        return 0;
    }
    return &g_fb;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (!g_fb_ready || x >= g_fb.width || y >= g_fb.height)
    {
        return;
    }

    uint8_t *base = (uint8_t *)(uintptr_t)g_fb.address;
    uint32_t offset = y * g_fb.pitch + x * (g_fb.bpp / 8u);
    uint32_t packed = pack_rgb(color);

    if (g_fb.bpp == 32)
    {
        *(uint32_t *)(base + offset) = packed;
    }
    else
    {
        base[offset + 0] = (uint8_t)(packed & 0xFF);
        base[offset + 1] = (uint8_t)((packed >> 8) & 0xFF);
        base[offset + 2] = (uint8_t)((packed >> 16) & 0xFF);
    }
}

void fb_clear(uint32_t color)
{
    if (!g_fb_ready)
    {
        return;
    }
    for (uint32_t y = 0; y < g_fb.height; y++)
    {
        for (uint32_t x = 0; x < g_fb.width; x++)
        {
            fb_put_pixel(x, y, color);
        }
    }
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!g_fb_ready)
    {
        return;
    }

    uint32_t x_end = x + w;
    uint32_t y_end = y + h;
    if (x_end > g_fb.width)
    {
        x_end = g_fb.width;
    }
    if (y_end > g_fb.height)
    {
        y_end = g_fb.height;
    }

    for (uint32_t yy = y; yy < y_end; yy++)
    {
        for (uint32_t xx = x; xx < x_end; xx++)
        {
            fb_put_pixel(xx, yy, color);
        }
    }
}

void fb_demo(void)
{
    if (!g_fb_ready)
    {
        return;
    }

    fb_clear(0x0D0D12);

    uint32_t bar_h = g_fb.height / 8u;
    for (uint32_t i = 0; i < 8; i++)
    {
        uint32_t color = (i * 32u) << 16 | (255u - i * 24u) << 8 | (64u + i * 16u);
        fb_fill_rect(0, i * bar_h, g_fb.width, bar_h, color);
    }

    uint32_t w = g_fb.width / 3u;
    uint32_t h = g_fb.height / 3u;
    fb_fill_rect(w / 2u, h / 2u, w, h, 0x1E90FF);
    fb_fill_rect(w, h, w, h, 0x32CD32);
    fb_fill_rect(w + w / 3u, h + h / 3u, w / 2u, h / 2u, 0xFF8C00);
}
