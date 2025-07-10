/*
 * display.c
 * FINALE KORREKTUR: Der Rahmen wird so gezeichnet, dass er den
 * 1-Pixel-Hardware-Versatz visuell exakt ausgleicht.
 */

#include "display.h"
#include "data.h"
#include <avr/pgmspace.h>
#include <string.h>
#include <stdint.h>
#include "ks0108.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

uint8_t pageBuf[SCREEN_W];
bool    displayInverted = false;

/* Icons und Glyphen bleiben unverändert */
static const uint8_t icoT[11] PROGMEM = {0x18,0x24,0x34,0x24,0x34,0x24,0x42,0x5A,0x5A,0x42,0x3C};
static const uint8_t icoP[11] PROGMEM = {0x08,0x08,0x2A,0x1C,0x08,0x00,0x10,0x38,0x54,0x10,0x10};
static const uint8_t glyphs[][FONT_W] PROGMEM = {
    {0x1F,0x11,0x1F},{0x11,0x1F,0x10},{0x1D,0x15,0x17},{0x15,0x15,0x1F},{0x07,0x04,0x1F},
    {0x17,0x15,0x1D},{0x1F,0x15,0x1D},{0x01,0x01,0x1F},{0x1F,0x15,0x1F},{0x17,0x15,0x1F},
    {0x00,0x10,0x00},{0x04,0x04,0x04},{0x06,0x09,0x06},{0x0E,0x11,0x11},{0x1F,0x11,0x0E},
    {0x1F,0x04,0x1F},{0x1F,0x05,0x07},{0x1E,0x05,0x1E}
};

static uint8_t glyphIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    switch (c) {
        case ',': return 10; case '-': return 11; case 'O': return 12; case 'C': return 13;
        case 'D': return 14; case 'H': return 15; case 'P': return 16; case 'A': return 17;
        default:  return 0xFF;
    }
}

static void drawLine(int x0, int y0, int x1, int y1, uint8_t pg) {
    int dx = ABS(x1-x0), sx = x0<x1?1:-1;
    int dy = -ABS(y1-y0), sy = y0<y1?1:-1;
    int err = dx+dy, yb = pg*8;
    while (1) {
        if (x0>=0 && x0<SCREEN_W && y0>=yb && y0<yb+8)
            pageBuf[x0] |= 1 << (y0-yb);
        if (x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if (e2>=dy) { err+=dy; x0+=sx; }
        if (e2<=dx) { err+=dx; y0+=sy; }
    }
}

static void drawBitmap(int x, int y, const uint8_t *bmp, uint8_t pg) {
    int yb = pg*8;
    for (uint8_t r=0; r<11; r++) {
        uint8_t bits = pgm_read_byte(&bmp[r]);
        int yy = y + r;
        if (yy<yb || yy>=yb+8) continue;
        for (uint8_t c=0; c<8; c++) if (bits & (0x80>>c)) {
            int xx = x + c;
            if (xx>=0 && xx<SCREEN_W) pageBuf[xx] |= 1 << (yy-yb);
        }
    }
}

static void drawChar(int x, int y, char ch, uint8_t pg) {
    uint8_t idx = glyphIndex(ch);
    if (idx==0xFF) return;
    int yb = pg*8;
    for (uint8_t col=0; col<FONT_W; col++) {
        uint8_t bits = pgm_read_byte(&glyphs[idx][col]);
        for (uint8_t b=0; b<FONT_H; b++) if (bits & (1<<b)) {
            int yy=y+b, xx=x+col;
            if (yy>=yb && yy<yb+8 && xx>=0 && xx<SCREEN_W) pageBuf[xx] |= 1 << (yy-yb);
        }
    }
}

static void drawString(int x, int y, const char *s, uint8_t pg) {
    while (*s) {
        drawChar(x, y, *s++, pg);
        x += FONT_W+1;
        if (x+FONT_W>=SCREEN_W) break;
    }
}

static void drawPlot16(const int16_t *data, uint8_t x0, uint8_t x1, int y0, int y1, uint8_t pg) {
    int h = y1 - y0;
    int len = x1 - x0 + 1;
    int16_t mn = INT16_MAX, mx = INT16_MIN;
    for (int i = 0; i < len; i++) {
        int16_t v = data[i];
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    int16_t range = (mx == mn ? 1 : mx - mn);
    int px = x0;
    int16_t v0 = data[0];
    int py = y0 + (mx - v0) * (uint32_t)h / range;
    for (int i = 1; i < len; i++) {
        int cx = x0 + i;
        int16_t v = data[i];
        int cy = y0 + (mx - v) * (uint32_t)h / range;
        drawLine(px, py, cx, cy, pg);
        px = cx; py = cy;
    }
}

void drawNumber(int x, int y, int16_t val, uint8_t dp, uint8_t pg) {
    int16_t v = val;
    uint16_t ab = v<0 ? -v : v;
    if (v<0) {
        drawChar(x, y, '-', pg);
        x += FONT_W+1;
    }
    uint16_t ip = dp ? ab/10 : ab;
    uint16_t fp = dp ? ab%10 : 0;
    char buf[5]; int bl = 0;
    do { buf[bl++] = '0' + ip%10; ip /= 10; } while (ip);
    while (bl--) {
        drawChar(x, y, buf[bl], pg);
        x += FONT_W+1;
    }
    if (dp) {
        drawChar(x, y, ',', pg);
        x += FONT_W+1;
        drawChar(x, y, '0'+fp, pg);
    }
}

void clearPage(uint8_t pg) {
    memset(pageBuf, 0, SCREEN_W);

    // Zeichne die vertikalen Linien für die aktuelle Page
    drawLine(0, pg*8, 0, pg*8+7, pg);
    drawLine(SCREEN_W-1, pg*8, SCREEN_W-1, pg*8+7, pg);

    // Zeichne die horizontalen Linien, aber nur in der letzten Page (pg=7)
    // Dies nutzt den 1-Pixel-Wrap-Around-Fehler gezielt aus.
    if (pg == SCREEN_H/8 - 1) { // Nur ausführen, wenn pg = 7
        // Zeichne die untere Linie bei y=62, damit sie bei y=63 erscheint.
        drawLine(0, SCREEN_H - 2, SCREEN_W - 1, SCREEN_H - 2, pg);
        
        // Zeichne die obere Linie bei y=63, damit sie nach y=0 "wrapt".
        drawLine(0, SCREEN_H - 1, SCREEN_W - 1, SCREEN_H - 1, pg);
    }
}

void renderScene(uint8_t scene, uint8_t pg) {
    const uint8_t order[5] = { 0, 2, 1, 3, 4 };
    uint8_t idx = scene < 5 ? scene : 0;
    scene = order[idx];

    PORTB |=  (1 << RW_PIN);
    CLR(PORTB, RW_PIN);

    if (scene < 4) {
        int yMid = (SCREEN_H - 11)/2;
        int xIcon = 1;
        if (scene < 2) {
            drawBitmap(xIcon, yMid, icoT, pg);
            drawString(xIcon + 9, yMid - 3, "OC", pg);
        } else {
            drawBitmap(xIcon, yMid, icoP, pg);
            drawString(xIcon + 9, yMid - 3, "HPA", pg);
        }
        const char *interval = (scene % 2 == 0) ? "24H" : "7D";
        drawString(xIcon + 9, yMid + FONT_H - 1, interval, pg);
        drawLine(PLOT_X0, PLOT_Y0, PLOT_X0, PLOT_Y1, pg);
        drawLine(PLOT_X0, PLOT_Y1, PLOT_X1, PLOT_Y1, pg);
        int ticks = (scene % 2 == 0) ? 13 : 8;
        for (int i = 0; i < ticks; i++) {
            int x = PLOT_X0 + i * (PLOT_X1 - PLOT_X0) / (ticks - 1);
            drawLine(x, PLOT_Y1-2, x, PLOT_Y1+2, pg);
        }
        int16_t mnv = INT16_MAX, mxv = INT16_MIN;
        int len = PLOT_X1 - PLOT_X0 + 1;
        for (int i = 0; i < len; i++) {
            int16_t v = dataGraph[i];
            if (v < mnv) mnv = v;
            if (v > mxv) mxv = v;
        }
        drawNumber(2, PLOT_Y0 - FONT_H + 3, mxv, 1, pg);
        drawNumber(2, PLOT_Y1 - FONT_H + 3, mnv, 1, pg);
        drawPlot16(dataGraph, PLOT_X0, PLOT_X1, PLOT_Y0, PLOT_Y1, pg);
    } else {
        int yMid = (SCREEN_H - 11)/2;
        drawBitmap(1, yMid, icoT, pg);
        drawNumber(12, yMid+2, dataT, 1, pg);
        drawString(12 + 5*(FONT_W+1), yMid+2, "OC", pg);
        drawBitmap(70, yMid, icoP, pg);
        drawNumber(81, yMid+2, (int16_t)dataP, 1, pg);
        drawString(81 + 7*(FONT_W+1), yMid+2, "HPA", pg);
    }
    PORTB &= ~(1 << RW_PIN);
}