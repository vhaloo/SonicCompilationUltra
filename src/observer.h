#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint8_t ram[65536], vram[65536], regs[32];
    uint32_t palettes[240][64];
    uint32_t background[240][320];
    int16_t scroll_a[240], scroll_b[240];
    int camera_x, camera_y;
} SCUSnapshot;
void scu_capture_line(int line, const uint16_t *pixels);
void scu_snapshot(SCUSnapshot *out);
void scu_ram_write8(unsigned address, uint8_t value);
void scu_ram_write16(unsigned address, uint16_t value);
#ifdef __cplusplus
}
#endif
