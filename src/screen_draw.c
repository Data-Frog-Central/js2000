#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <gateway.h>

#include "screen_draw.h"
#include "font8x8.h"
#include "font5x8.h"

void screen_draw_text_8x8(uint32_t *fb, int fb_width, int fb_height, int x, int y, const char *text, uint32_t color) {
    for (const char *p = text; *p; ++p) {
        unsigned char ch = (unsigned char)*p;
        if (ch < 32 || ch > 127) ch = '?';
        const uint8_t *glyph = font8x8_basic[ch - 32];
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                if (glyph[row] & (1 << (7 - col))) {
                    int px = x + col;
                    int py = y + row;
                    if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                        fb[py * fb_width + px] = color | 0xFF000000;
                    }
                }
            }
        }
        x += 8;
    }
}

void screen_draw_text_5x8(uint32_t *fb, int fb_width, int fb_height, int x, int y, const char *text, uint32_t color) {
    for (const char *p = text; *p; ++p) {
        unsigned char ch = (unsigned char)*p;
        if (ch < 32 || ch > 127) ch = '?';
        const uint8_t *glyph = font5x8_basic[ch - 32];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 8; ++row) {
                if (bits & (1 << row)) {
                    int px = x + col;
                    int py = y + row;
                    if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                        fb[py * fb_width + px] = color | 0xFF000000;
                    }
                }
            }
        }
        x += 6; // 5px width + 1px spacing
    }
}

// Draw a filled rectangle
void screen_draw_fill_rect(uint32_t *fb, int fb_width, int fb_height, int x, int y, int w, int h, uint32_t color) {
    if (x >= fb_width || y >= fb_height || x + w <= 0 || y + h <= 0) return; // Off-screen checks
    
    if (x < 0) { w += x; x = 0; } // Clip left boundary
    if (y < 0) { h += y; y = 0; } // Clip top boundary
    if (x + w > fb_width) w = fb_width - x; // Clip right boundary
    if (y + h > fb_height) h = fb_height - y; // Clip bottom boundary
    if (w <= 0 || h <= 0) return; // Safety check after clipping
    
    int area = w * h;
    uint32_t final_color = color | 0xFF000000;

    if (!accel_funcs.hcge_fb_fill_rect || area < 512) { // Maybe figure out a better way to guess
        for (int py = y; py < y + h; ++py) {
            uint32_t *row = &fb[py * fb_width + x];
            for (int px = 0; px < w; ++px) {
                row[px] = final_color;
            }
        }
        return;
    }

    if (accel_funcs.hcge_fb_fill_rect) {
        FrameInfo fill_frame = {
            .frame = fb,
            .full_width = fb_width,
            .full_height = fb_height,
            .width = w,
            .height = h,
            .pitch = fb_width * sizeof(uint32_t),
            .x = x,
            .y = y,
            .rgb32 = true
        };
        accel_funcs.hcge_fb_fill_rect(fill_frame, final_color);
        return;
    }
}

// Draw a rectangle outline
void screen_draw_rect(uint32_t *fb, int fb_width, int fb_height, int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    screen_draw_fill_rect(fb, fb_width, fb_height, x, y, w, 1, color); // Top edge (width x 1)
    screen_draw_fill_rect(fb, fb_width, fb_height, x, y + h - 1, w, 1, color); // Bottom edge (width x 1)
    screen_draw_fill_rect(fb, fb_width, fb_height, x, y, 1, h, color); // Left edge (1 x height)
    screen_draw_fill_rect(fb, fb_width, fb_height, x + w - 1, y, 1, h, color); // Right edge (1 x height)
}

void screen_draw_line(uint32_t *fb, int fb_width, int fb_height, int x0, int y0, int x1, int y1, uint32_t color) {
    if (y0 == y1) { // Check if the line is strictly horizontal
        int x = (x0 < x1) ? x0 : x1;
        int w = abs(x1 - x0) + 1;
        screen_draw_fill_rect(fb, fb_width, fb_height, x, y0, w, 1, color);
        return;
    }

    if (x0 == x1) { // Check if the line is strictly vertical
        int y = (y0 < y1) ? y0 : y1;
        int h = abs(y1 - y0) + 1;
        screen_draw_fill_rect(fb, fb_width, fb_height, x0, y, 1, h, color);
        return;
    }

    // Draw a line (Bresenham's algorithm)
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        if (x0 >= 0 && x0 < fb_width && y0 >= 0 && y0 < fb_height)
            fb[y0 * fb_width + x0] = color | 0xFF000000;
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Draw a raw 32bpp image (no format conversion)
void screen_draw_image(uint32_t *fb, int fb_width, int fb_height, int x, int y, const uint32_t *img, int img_w, int img_h) {
    for (int iy = 0; iy < img_h; ++iy) {
        int py = y + iy;
        if (py < 0 || py >= fb_height) continue;
        for (int ix = 0; ix < img_w; ++ix) {
            int px = x + ix;
            if (px < 0 || px >= fb_width) continue;
            fb[py * fb_width + px] = img[iy * img_w + ix];
        }
    }
}

void screen_draw_copy_framebuffer(uint32_t *fb, int fb_width, int fb_height, void *out_buf) {
    memcpy(out_buf, fb, fb_width * fb_height * 4);
}
