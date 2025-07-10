/*
 * display.c
 * 
 * Display-Rendering-Funktionen für das KS0108 128x64 Grafik-LCD
 * Zeichnet Graphen, Zahlen, Texte und Icons auf dem Display
 * 
 * FINALE KORREKTUR: Der Rahmen wird so gezeichnet, dass er den
 * 1-Pixel-Hardware-Versatz visuell exakt ausgleicht.
 */

#include "display.h"
#include "data.h"
#include <avr/pgmspace.h>
#include <string.h>
#include <stdint.h>
#include "ks0108.h"

// Makro für absoluten Wert (vermeidet negative Zahlen)
#define ABS(x) ((x) < 0 ? -(x) : (x))

// Globale Variablen für Display-Buffer und Inversion
uint8_t pageBuf[SCREEN_W];  // Puffer für eine Display-Seite (128 Bytes)
bool    displayInverted = false;  // Inversion ein/aus (aktuell nicht verwendet)

/* Icons und Glyphen - im Programmspeicher gespeichert (Flash-ROM) */
// Temperatur-Icon (11x11 Pixel)
static const uint8_t icoT[11] PROGMEM = {0x18,0x24,0x34,0x24,0x34,0x24,0x42,0x5A,0x5A,0x42,0x3C};
// Druck-Icon (11x11 Pixel)  
static const uint8_t icoP[11] PROGMEM = {0x08,0x08,0x2A,0x1C,0x08,0x00,0x10,0x38,0x54,0x10,0x10};

// Font-Glyphen für Zahlen und Sonderzeichen (3x5 Pixel pro Zeichen)
static const uint8_t glyphs[][FONT_W] PROGMEM = {
    {0x1F,0x11,0x1F},{0x11,0x1F,0x10},{0x1D,0x15,0x17},{0x15,0x15,0x1F},{0x07,0x04,0x1F},  // 0-4
    {0x17,0x15,0x1D},{0x1F,0x15,0x1D},{0x01,0x01,0x1F},{0x1F,0x15,0x1F},{0x17,0x15,0x1F},  // 5-9
    {0x00,0x10,0x00},{0x04,0x04,0x04},{0x06,0x09,0x06},{0x0E,0x11,0x11},{0x1F,0x11,0x0E},  // ,-O
    {0x1F,0x04,0x1F},{0x1F,0x05,0x07},{0x1E,0x05,0x1E}  // C-D-H-P-A
};

// Konvertiert ein Zeichen in den entsprechenden Glyphen-Index
static uint8_t glyphIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';  // Zahlen 0-9
    switch (c) {
        case ',': return 10; case '-': return 11; case 'O': return 12; case 'C': return 13;
        case 'D': return 14; case 'H': return 15; case 'P': return 16; case 'A': return 17;
        default:  return 0xFF;  // Ungültiges Zeichen
    }
}

// Zeichnet eine Linie zwischen zwei Punkten (Bresenham-Algorithmus)
static void drawLine(int x0, int y0, int x1, int y1, uint8_t pg) {
    int dx = ABS(x1-x0), sx = x0<x1?1:-1;  // Delta X und Schrittrichtung X
    int dy = -ABS(y1-y0), sy = y0<y1?1:-1; // Delta Y und Schrittrichtung Y
    int err = dx+dy, yb = pg*8;  // Fehler und Y-Basis für aktuelle Seite
    
    while (1) {
        // Pixel setzen, falls innerhalb der Display-Grenzen
        if (x0>=0 && x0<SCREEN_W && y0>=yb && y0<yb+8)
            pageBuf[x0] |= 1 << (y0-yb);  // Bit in pageBuf setzen
        
        // Ende erreicht?
        if (x0==x1 && y0==y1) break;
        
        // Bresenham-Algorithmus: Fehler aktualisieren
        int e2 = 2*err;
        if (e2>=dy) { err+=dy; x0+=sx; }  // X-Schritt
        if (e2<=dx) { err+=dx; y0+=sy; }  // Y-Schritt
    }
}

// Zeichnet ein Bitmap (Icon) an der angegebenen Position
static void drawBitmap(int x, int y, const uint8_t *bmp, uint8_t pg) {
    int yb = pg*8;  // Y-Basis für aktuelle Seite
    
    // 11 Zeilen des Icons zeichnen
    for (uint8_t r=0; r<11; r++) {
        uint8_t bits = pgm_read_byte(&bmp[r]);  // Byte aus Flash lesen
        int yy = y + r;  // Y-Position für aktuelle Zeile
        
        // Prüfen, ob Zeile innerhalb der aktuellen Seite liegt
        if (yy<yb || yy>=yb+8) continue;
        
        // 8 Bits pro Zeile verarbeiten
        for (uint8_t c=0; c<8; c++) if (bits & (0x80>>c)) {
            int xx = x + c;  // X-Position für aktuelles Bit
            if (xx>=0 && xx<SCREEN_W) pageBuf[xx] |= 1 << (yy-yb);  // Pixel setzen
        }
    }
}

// Zeichnet ein einzelnes Zeichen (3x5 Pixel)
static void drawChar(int x, int y, char ch, uint8_t pg) {
    uint8_t idx = glyphIndex(ch);  // Glyphen-Index ermitteln
    if (idx==0xFF) return;  // Ungültiges Zeichen ignorieren
    
    int yb = pg*8;  // Y-Basis für aktuelle Seite
    
    // 3 Spalten des Zeichens zeichnen
    for (uint8_t col=0; col<FONT_W; col++) {
        uint8_t bits = pgm_read_byte(&glyphs[idx][col]);  // Spalte aus Flash lesen
        
        // 5 Bits pro Spalte verarbeiten
        for (uint8_t b=0; b<FONT_H; b++) if (bits & (1<<b)) {
            int yy=y+b, xx=x+col;  // Pixel-Position berechnen
            if (yy>=yb && yy<yb+8 && xx>=0 && xx<SCREEN_W) 
                pageBuf[xx] |= 1 << (yy-yb);  // Pixel setzen
        }
    }
}

// Zeichnet einen String (mehrere Zeichen hintereinander)
static void drawString(int x, int y, const char *s, uint8_t pg) {
    while (*s) {  // Solange String nicht zu Ende
        drawChar(x, y, *s++, pg);  // Zeichen zeichnen
        x += FONT_W+1;  // X-Position für nächstes Zeichen
        if (x+FONT_W>=SCREEN_W) break;  // Display-Rand erreicht?
    }
}

// Zeichnet einen Graphen aus 16-bit-Daten
static void drawPlot16(const int16_t *data, uint8_t x0, uint8_t x1, int y0, int y1, uint8_t pg) {
    int h = y1 - y0;  // Höhe des Graphen
    int len = x1 - x0 + 1;  // Anzahl der Datenpunkte
    int16_t mn = INT16_MAX, mx = INT16_MIN;  // Min/Max-Werte
    
    // Min/Max-Werte finden
    for (int i = 0; i < len; i++) {
        int16_t v = data[i];
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    
    int16_t range = (mx == mn ? 1 : mx - mn);  // Wertebereich (mindestens 1)
    int px = x0;  // Start-X-Position
    int16_t v0 = data[0];  // Erster Wert
    int py = y0 + (mx - v0) * (uint32_t)h / range;  // Start-Y-Position
    
    // Linien zwischen allen Datenpunkten zeichnen
    for (int i = 1; i < len; i++) {
        int cx = x0 + i;  // Aktuelle X-Position
        int16_t v = data[i];  // Aktueller Wert
        int cy = y0 + (mx - v) * (uint32_t)h / range;  // Aktuelle Y-Position
        drawLine(px, py, cx, cy, pg);  // Linie zeichnen
        px = cx; py = cy;  // Position für nächste Linie
    }
}

// Zeichnet eine Zahl mit optionalen Dezimalstellen
void drawNumber(int x, int y, int16_t val, uint8_t dp, uint8_t pg) {
    int16_t v = val;  // Lokale Kopie des Wertes
    uint16_t ab = v<0 ? -v : v;  // Absoluter Wert
    
    // Minuszeichen zeichnen, falls negativ
    if (v<0) {
        drawChar(x, y, '-', pg);
        x += FONT_W+1;  // X-Position für nächste Ziffer
    }
    
    uint16_t ip = dp ? ab/10 : ab;  // Ganzzahl-Teil
    uint16_t fp = dp ? ab%10 : 0;   // Dezimal-Teil
    
    char buf[5]; int bl = 0;  // Puffer für Ziffern
    
    // Ganzzahl in String umwandeln (rückwärts)
    do { 
        buf[bl++] = '0' + ip%10; 
        ip /= 10; 
    } while (ip);
    
    // Ziffern zeichnen (vorwärts, da rückwärts gespeichert)
    while (bl--) {
        drawChar(x, y, buf[bl], pg);
        x += FONT_W+1;  // X-Position für nächste Ziffer
    }
    
    // Dezimalstelle zeichnen, falls gewünscht
    if (dp) {
        drawChar(x, y, ',', pg);  // Komma
        x += FONT_W+1;
        drawChar(x, y, '0'+fp, pg);  // Dezimalstelle
    }
}

// Löscht eine Display-Seite und zeichnet den Rahmen
void clearPage(uint8_t pg) {
    memset(pageBuf, 0, SCREEN_W);  // Puffer komplett löschen

    // Vertikale Linien für die aktuelle Seite zeichnen
    drawLine(0, pg*8, 0, pg*8+7, pg);           // Linke Linie
    drawLine(SCREEN_W-1, pg*8, SCREEN_W-1, pg*8+7, pg);  // Rechte Linie

    // Horizontale Linien nur in der letzten Seite (pg=7)
    // Nutzt den 1-Pixel-Wrap-Around-Fehler gezielt aus
    if (pg == SCREEN_H/8 - 1) {  // Nur ausführen, wenn pg = 7
        // Untere Linie bei y=62 zeichnen, damit sie bei y=63 erscheint
        drawLine(0, SCREEN_H - 2, SCREEN_W - 1, SCREEN_H - 2, pg);
        
        // Obere Linie bei y=63 zeichnen, damit sie nach y=0 "wrapt"
        drawLine(0, SCREEN_H - 1, SCREEN_W - 1, SCREEN_H - 1, pg);
    }
}

// Rendert eine Szene (Seite) in die angegebene Display-Seite
void renderScene(uint8_t scene, uint8_t pg) {
    // Reihenfolge der Szenen: 0,2,1,3,4 (entspricht Seiten 1,3,2,4,5)
    const uint8_t order[5] = { 0, 2, 1, 3, 4 };
    uint8_t idx = scene < 5 ? scene : 0;
    scene = order[idx];

    // RW-Pin für Display-Operation setzen
    PORTB |=  (1 << RW_PIN);
    CLR(PORTB, RW_PIN);

    // Seiten 1-4: Graphen mit Icons und Beschriftungen
    if (scene < 4) {
        int yMid = (SCREEN_H - 11)/2;  // Y-Mitte für Icon-Position
        int xIcon = 1;  // X-Position für Icon
        
        // Icon und Beschriftung je nach Seite
        if (scene < 2) {
            // Temperatur-Seiten (0,1)
            drawBitmap(xIcon, yMid, icoT, pg);  // Temperatur-Icon
            drawString(xIcon + 9, yMid - 3, "OC", pg);  // °C-Beschriftung
        } else {
            // Druck-Seiten (2,3)
            drawBitmap(xIcon, yMid, icoP, pg);  // Druck-Icon
            drawString(xIcon + 9, yMid - 3, "HPA", pg);  // hPa-Beschriftung
        }
        
        // Zeitintervall-Beschriftung
        const char *interval = (scene % 2 == 0) ? "24H" : "7D";
        drawString(xIcon + 9, yMid + FONT_H - 1, interval, pg);
        
        // Graph-Rahmen zeichnen
        drawLine(PLOT_X0, PLOT_Y0, PLOT_X0, PLOT_Y1, pg);  // Linke Achse
        drawLine(PLOT_X0, PLOT_Y1, PLOT_X1, PLOT_Y1, pg);  // Untere Achse
        
        // Skalierungsstriche auf X-Achse
        int ticks = (scene % 2 == 0) ? 13 : 8;  // Anzahl Striche (24h vs 7d)
        for (int i = 0; i < ticks; i++) {
            int x = PLOT_X0 + i * (PLOT_X1 - PLOT_X0) / (ticks - 1);
            drawLine(x, PLOT_Y1-2, x, PLOT_Y1+2, pg);  // Kurzer Strich
        }
        
        // Min/Max-Werte für Y-Achse finden
        int16_t mnv = INT16_MAX, mxv = INT16_MIN;
        int len = PLOT_X1 - PLOT_X0 + 1;
        for (int i = 0; i < len; i++) {
            int16_t v = dataGraph[i];
            if (v < mnv) mnv = v;
            if (v > mxv) mxv = v;
        }
        
        // Min/Max-Werte an Y-Achse anzeigen
        drawNumber(2, PLOT_Y0 - FONT_H + 3, mxv, 1, pg);  // Max-Wert oben
        drawNumber(2, PLOT_Y1 - FONT_H + 3, mnv, 1, pg);  // Min-Wert unten
        
        // Graph zeichnen
        drawPlot16(dataGraph, PLOT_X0, PLOT_X1, PLOT_Y0, PLOT_Y1, pg);
        
    } else {
        // Seite 5: Aktuelle Werte (kein Graph)
        int yMid = (SCREEN_H - 11)/2;  // Y-Mitte für Icon-Position
        
        // Temperatur anzeigen
        drawBitmap(1, yMid, icoT, pg);  // Temperatur-Icon
        drawNumber(12, yMid+2, dataT, 1, pg);  // Temperatur-Wert
        drawString(12 + 5*(FONT_W+1), yMid+2, "OC", pg);  // °C-Beschriftung
        
        // Druck anzeigen
        drawBitmap(70, yMid, icoP, pg);  // Druck-Icon
        drawNumber(81, yMid+2, (int16_t)dataP, 1, pg);  // Druck-Wert
        drawString(81 + 7*(FONT_W+1), yMid+2, "HPA", pg);  // hPa-Beschriftung
    }
    
    // RW-Pin zurücksetzen
    PORTB &= ~(1 << RW_PIN);
}