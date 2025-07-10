/*
 * display.h
 *
 * Header-Datei für Display-Treiber (KS0108 LCD-Controller)
 * Definiert die Schnittstelle für High-Level Display-Funktionen
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

// Display-Dimensionen für KS0108 LCD
// Das Display ist in 8 Seiten (Pages) zu je 8 Pixeln Höhe aufgeteilt
#define DISPLAY_WIDTH    128   // Breite in Pixeln
#define DISPLAY_HEIGHT   64    // Höhe in Pixeln
#define DISPLAY_PAGES    8     // Anzahl der Seiten (64/8 = 8)

// Display-Bereiche für verschiedene Anzeigen
// Diese Bereiche definieren wo welche Informationen angezeigt werden
#define DISPLAY_HEADER_HEIGHT  8     // Höhe des Header-Bereichs (1 Seite)
#define DISPLAY_GRAPH_HEIGHT   48    // Höhe des Graphen-Bereichs (6 Seiten)
#define DISPLAY_FOOTER_HEIGHT  8     // Höhe des Footer-Bereichs (1 Seite)

// Graph-Dimensionen für Temperatur- und Druck-Anzeige
#define GRAPH_WIDTH      120   // Breite des Graphen (mit Rand)
#define GRAPH_HEIGHT     40    // Höhe des Graphen
#define GRAPH_X_OFFSET   4     // X-Offset vom linken Rand
#define GRAPH_Y_OFFSET   12    // Y-Offset vom oberen Rand

// Text-Positionen für verschiedene Anzeigen
#define TEXT_TEMP_X      0     // X-Position für Temperatur-Text
#define TEXT_TEMP_Y      0     // Y-Position für Temperatur-Text
#define TEXT_PRESSURE_X  64    // X-Position für Druck-Text (rechte Hälfte)
#define TEXT_PRESSURE_Y  0     // Y-Position für Druck-Text
#define TEXT_TIME_X      0     // X-Position für Zeit-Anzeige
#define TEXT_TIME_Y      56    // Y-Position für Zeit-Anzeige (unten)

// Farben für das Display (Monochrom)
#define COLOR_BLACK      0     // Schwarzer Pixel (an)
#define COLOR_WHITE      1     // Weißer Pixel (aus)

// Zeichensatz-Definitionen
// Standard-Zeichensatz für 6x8 Pixel Zeichen
#define CHAR_WIDTH       6     // Breite eines Zeichens in Pixeln
#define CHAR_HEIGHT      8     // Höhe eines Zeichens in Pixeln
#define CHARS_PER_LINE   21    // Maximale Zeichen pro Zeile (128/6)

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

// High-Level Display-Funktionen
// Diese Funktionen bieten einfache Schnittstellen für Anwendungen

// Display initialisieren und konfigurieren
// Führt alle notwendigen Initialisierungsschritte durch
void display_init(void);

// Display löschen
// Löscht das gesamte Display
void display_clear(void);

// Text an Position ausgeben
// Zeichnet einen String an der angegebenen Position
void display_text(uint8_t x, uint8_t y, const char* text);

// Zahl an Position ausgeben
// Zeichnet eine Zahl an der angegebenen Position
void display_number(uint8_t x, uint8_t y, int16_t number);

// Temperatur anzeigen
// Zeichnet Temperatur mit Einheit und Formatierung
void display_temperature(int16_t temp);

// Druck anzeigen
// Zeichnet Druck mit Einheit und Formatierung
void display_pressure(uint16_t pressure);

// Luftfeuchtigkeit anzeigen
// Zeichnet Luftfeuchtigkeit mit Einheit und Formatierung
void display_humidity(uint16_t humidity);

// Zeit anzeigen
// Zeichnet aktuelle Zeit im Format HH:MM
void display_time(uint8_t hour, uint8_t minute);

// Datum anzeigen
// Zeichnet aktuelles Datum im Format DD.MM
void display_date(uint8_t day, uint8_t month);

// Linie zeichnen
// Zeichnet eine Linie von (x1,y1) nach (x2,y2)
void display_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

// Rechteck zeichnen
// Zeichnet ein Rechteck mit den angegebenen Koordinaten
void display_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

// Gefülltes Rechteck zeichnen
// Zeichnet ein gefülltes Rechteck
void display_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

// Kreis zeichnen
// Zeichnet einen Kreis mit Mittelpunkt (x,y) und Radius r
void display_circle(uint8_t x, uint8_t y, uint8_t radius);

// Graph zeichnen
// Zeichnet einen Liniengraphen mit den angegebenen Daten
void display_graph(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                   const int16_t* data, uint8_t data_count);

// Temperatur-Graph zeichnen
// Zeichnet speziell formatierte Temperatur-Daten
void display_temp_graph(const int16_t* temp_data, uint8_t count);

// Druck-Graph zeichnen
// Zeichnet speziell formatierte Druck-Daten
void display_pressure_graph(const uint16_t* pressure_data, uint8_t count);

// Hauptanzeige aktualisieren
// Zeichnet die komplette Hauptanzeige mit aktuellen Werten
void display_update_main(int16_t temp, uint16_t pressure, uint16_t humidity);

// Graph-Anzeige aktualisieren
// Zeichnet die Graph-Anzeige mit historischen Daten
void display_update_graph(const int16_t* temp_data, const uint16_t* pressure_data, uint8_t count);

// Display-Timeout verwalten
// Schaltet das Display nach einer bestimmten Zeit aus
void display_set_timeout(uint16_t seconds);
uint8_t display_check_timeout(void);

// Display ein-/ausschalten
// Kontrolliert die Display-Beleuchtung
void display_on(void);
void display_off(void);

// Display-Kontrast einstellen
// Passt den Kontrast des Displays an (falls unterstützt)
void display_set_contrast(uint8_t contrast);

#endif /* DISPLAY_H_ */
