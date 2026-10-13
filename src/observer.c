/* Read-only presentation observer. Nothing here changes CPU, VDP or timing.
 * Explicit RAM writes are used only by opt-in gameplay assistance. */
#include <libretro.h>
#include "shared.h"
#include "observer.h"
static uint32_t line_palettes[240][64];
static uint32_t line_background[240][320];
static int16_t line_a[240], line_b[240];
void scu_capture_line(int line, const uint16_t *pixels) {
    unsigned i, scroll, mode, width, height, base, x;
    if (line < 0 || line >= 240)
        return;
    for (i = 0; i < 64; i++) {
        uint16_t p = pixels[0x40 + i];
        unsigned r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31;
        line_palettes[line][i] =
            0xFF000000u | ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
    }
    mode = reg[11] & 3;
    scroll = (reg[13] & 63) * 1024u;
    if (mode == 3)
        scroll += line * 4;
    else if (mode == 2)
        scroll += (line & ~7) * 4;
    line_a[line] = (int16_t)*(uint16_t *)(vram + (scroll & 65535));
    line_b[line] = (int16_t)*(uint16_t *)(vram + ((scroll + 2) & 65535));
    /* Sample only Plane B at its real raster state. Sprite/foreground pixels
     * never enter the stretched backdrop. Keep native column scrolling. */
    width = (reg[16] & 3) == 1 ? 64 : (reg[16] & 3) == 3 ? 128 : 32;
    height = ((reg[16] >> 4) & 3) == 1 ? 64 : ((reg[16] >> 4) & 3) == 3 ? 128 : 32;
    base = (reg[4] & 7) * 8192u;
    for (x = 0; x < 320; x++) {
        int bx = (int)x - line_b[line], by = line + *(uint16_t *)(vsram + 2);
        unsigned cell, attr, ix, iy, byte, color;
        if (reg[11] & 4) {
            unsigned column = (x >> 4) * 4 + 2;
            if (column < 0x80)
                by = line + *(uint16_t *)(vsram + column);
        }
        cell =
            (base + ((((by >> 3) & (height - 1)) * width + ((bx >> 3) & (width - 1))) * 2)) & 65535;
        attr = *(uint16_t *)(vram + cell);
        ix = (bx & 7) ^ ((attr & 0x800) ? 7 : 0);
        iy = (by & 7) ^ ((attr & 0x1000) ? 7 : 0);
        byte = vram[(((attr & 2047) * 32 + iy * 4 + ix / 2) & 65535) ^ 1];
        color = (ix & 1) ? byte & 15 : byte >> 4;
        if (!color)
            color = 0x40;
        else {
            color |= (attr >> 9) & 48;
            if (!(reg[12] & 8) || (attr & 0x8000))
                color |= 0x40;
        }
        {
            uint16_t p = pixels[color];
            unsigned r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31;
            line_background[line][x] =
                0xFF000000u | ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
        }
    }
}
void scu_snapshot(SCUSnapshot *out) {
    unsigned i;
    for (i = 0; i < 65536; i++) {
        out->ram[i] = work_ram[i ^ 1];
        out->vram[i] = vram[i ^ 1];
    }
    memcpy(out->regs, reg, 32);
    memcpy(out->palettes, line_palettes, sizeof line_palettes);
    memcpy(out->background, line_background, sizeof line_background);
    memcpy(out->scroll_a, line_a, sizeof line_a);
    memcpy(out->scroll_b, line_b, sizeof line_b);
}
void scu_ram_write8(unsigned a, uint8_t value) {
    work_ram[(a & 65535) ^ 1] = value;
}
void scu_ram_write16(unsigned a, uint16_t value) {
    scu_ram_write8(a, value >> 8);
    scu_ram_write8(a + 1, value & 255);
}
