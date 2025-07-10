/*
 * ks0108.h
 *
 * Header-Datei für KS0108 LCD-Controller Treiber
 * Definiert die Low-Level-Schnittstelle für das 128x64 Grafikdisplay
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 

#ifndef KS0108_H_
#define KS0108_H_

#include <stdint.h>

// KS0108 LCD-Controller Kommandos
// Diese Kommandos werden an den KS0108-Controller gesendet
#define KS0108_CMD_DISPLAY_ON     0x3F  // Display einschalten
#define KS0108_CMD_DISPLAY_OFF    0x3E  // Display ausschalten
#define KS0108_CMD_SET_PAGE       0xB8  // Seitenadresse setzen (0-7)
#define KS0108_CMD_SET_COLUMN     0x40  // Spaltenadresse setzen (0-63)
#define KS0108_CMD_SET_START_LINE 0xC0  // Startzeile setzen (0-63)
#define KS0108_CMD_READ_STATUS    0x00  // Status-Register lesen
#define KS0108_CMD_READ_DATA      0x01  // Daten-Register lesen
#define KS0108_CMD_WRITE_DATA     0x02  // Daten-Register schreiben

// KS0108 Status-Bits
// Diese Bits werden im Status-Register verwendet
#define KS0108_STATUS_BUSY        0x80  // Busy-Flag (1=Controller beschäftigt)
#define KS0108_STATUS_ON_OFF      0x20  // Display On/Off Status
#define KS0108_STATUS_RESET       0x10  // Reset-Status

// Display-Dimensionen für KS0108
// Das Display ist in 8 Seiten (Pages) zu je 8 Pixeln Höhe aufgeteilt
#define KS0108_WIDTH      128   // Gesamtbreite in Pixeln
#define KS0108_HEIGHT     64    // Gesamthöhe in Pixeln
#define KS0108_PAGES      8     // Anzahl der Seiten (64/8 = 8)
#define KS0108_COLUMNS    64    // Spalten pro Controller-Chip

// Chip-Select-Definitionen
// Das Display hat zwei KS0108-Controller (links und rechts)
#define KS0108_CHIP_LEFT  0     // Linker Controller (Spalten 0-63)
#define KS0108_CHIP_RIGHT 1     // Rechter Controller (Spalten 64-127)

// Timing-Konstanten für KS0108
// Diese Werte bestimmen die Geschwindigkeit der Display-Kommunikation
#define KS0108_ENABLE_PULSE_US    1     // Enable-Puls-Dauer in Mikrosekunden
#define KS0108_SETUP_TIME_US      1     // Setup-Zeit vor Enable-Puls
#define KS0108_HOLD_TIME_US       1     // Hold-Zeit nach Enable-Puls

// Low-Level Display-Funktionen
// Diese Funktionen kommunizieren direkt mit dem KS0108-Controller

// Display initialisieren
// Konfiguriert alle Pins und startet das Display
void lcd_init(void);

// Display-Chip auswählen (0 = links, 1 = rechts)
// Wählt die entsprechende Display-Hälfte für Operationen
void lcd_select_chip(uint8_t chip);

// Kommando an Display senden
// Sendet ein Steuerkommando an den KS0108-Controller
void lcd_write_cmd(uint8_t cmd);

// Daten an Display senden
// Sendet Bilddaten an das Display
void lcd_write_data(uint8_t data);

// Daten vom Display lesen
// Liest aktuelle Bilddaten vom Display
uint8_t lcd_read_data(void);

// Display-Status lesen
// Prüft ob das Display bereit ist
uint8_t lcd_read_status(void);

// Warten bis Display bereit ist
// Wartet bis das Display nicht mehr beschäftigt ist
void lcd_wait_ready(void);

// Display komplett löschen
// Setzt alle Pixel auf weiß
void lcd_clear(void);

// Einzelnen Pixel setzen
// Setzt einen Pixel an Position (x,y) auf schwarz
void lcd_set_pixel(uint8_t x, uint8_t y);

// Einzelnen Pixel löschen
// Setzt einen Pixel an Position (x,y) auf weiß
void lcd_clear_pixel(uint8_t x, uint8_t y);

// Display-Seite setzen
// Setzt die aktuelle Seitenadresse (0-7)
void lcd_set_page(uint8_t page);

// Display-Spalte setzen
// Setzt die aktuelle Spaltenadresse (0-63 pro Chip)
void lcd_set_column(uint8_t column);

// Display-Startzeile setzen
// Setzt die Startzeile für vertikales Scrolling (0-63)
void lcd_set_start_line(uint8_t line);

// Display ein-/ausschalten
// Kontrolliert die Display-Anzeige
void lcd_display_on(void);
void lcd_display_off(void);

// Display-Inversion ein-/ausschalten
// Invertiert alle Pixel (schwarz/weiß tauschen)
void lcd_invert_display(uint8_t invert);

// Display-Kontrast einstellen
// Passt den Kontrast des Displays an (falls unterstützt)
void lcd_set_contrast(uint8_t contrast);

// Display-Power-Management
// Kontrolliert den Stromverbrauch des Displays
void lcd_power_save(uint8_t enable);

// Display-Reset durchführen
// Führt einen Hardware-Reset des Displays durch
void lcd_reset(void);

// Display-Selbsttest
// Führt einen Selbsttest des Displays durch
uint8_t lcd_self_test(void);

// Display-Informationen abrufen
// Liest Informationen über das Display
typedef struct {
	uint8_t  controller_count;  // Anzahl der Controller (normalerweise 2)
	uint8_t  width;             // Display-Breite in Pixeln
	uint8_t  height;            // Display-Höhe in Pixeln
	uint8_t  pages;             // Anzahl der Seiten
	uint8_t  columns_per_chip;  // Spalten pro Controller
	uint8_t  is_initialized;    // Initialisierungsstatus
	uint8_t  is_on;             // Display-An/Aus-Status
	uint8_t  is_inverted;       // Inversionsstatus
} lcd_info_t;

// Display-Informationen lesen
// Liest aktuelle Display-Konfiguration
void lcd_get_info(lcd_info_t* info);

// Display-Statistiken
// Gibt Informationen über Display-Nutzung zurück
typedef struct {
	uint16_t pixels_set;        // Anzahl gesetzter Pixel
	uint16_t pixels_cleared;    // Anzahl gelöschter Pixel
	uint16_t commands_sent;     // Anzahl gesendeter Kommandos
	uint16_t data_bytes_sent;   // Anzahl gesendeter Datenbytes
	uint16_t read_operations;   // Anzahl Leseoperationen
	uint16_t error_count;       // Anzahl aufgetretener Fehler
} lcd_stats_t;

// Display-Statistiken abrufen
// Liest aktuelle Nutzungsstatistiken
void lcd_get_stats(lcd_stats_t* stats);

// Display-Statistiken zurücksetzen
// Setzt alle Zähler zurück
void lcd_reset_stats(void);

// Display-Bereich löschen
// Löscht einen rechteckigen Bereich des Displays
void lcd_clear_area(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

// Display-Bereich kopieren
// Kopiert einen Bereich an eine andere Position
void lcd_copy_area(uint8_t src_x, uint8_t src_y, uint8_t dest_x, uint8_t dest_y, 
                   uint8_t width, uint8_t height);

// Display-Scroll-Funktionen
// Scrollt das Display in verschiedene Richtungen
void lcd_scroll_up(uint8_t lines);
void lcd_scroll_down(uint8_t lines);
void lcd_scroll_left(uint8_t pixels);
void lcd_scroll_right(uint8_t pixels);

// Display-Frame-Buffer-Funktionen
// Verwaltet einen lokalen Frame-Buffer für effiziente Updates
void lcd_fb_init(void);
void lcd_fb_clear(void);
void lcd_fb_set_pixel(uint8_t x, uint8_t y);
void lcd_fb_clear_pixel(uint8_t x, uint8_t y);
void lcd_fb_update(void);

#endif /* KS0108_H_ */
