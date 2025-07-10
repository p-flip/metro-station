/*
 * ks0108.c
 *
 * KS0108 LCD-Controller Treiber für 128x64 Grafikdisplay
 * Implementiert die Low-Level-Kommunikation mit dem Display
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 
//#define F_CPU 3686400UL

#include <avr/io.h>
#include <util/delay.h>
#include "ks0108.h"

// Pin-Definitionen für KS0108 LCD-Controller
// Diese Pins steuern die Kommunikation mit dem Display
#define LCD_DDR   DDRC    // Data Direction Register für LCD-Pins
#define LCD_PORT  PORTC   // Port Register für LCD-Pins
#define LCD_PIN   PINC    // Pin Register für LCD-Pins (zum Lesen)

// LCD-Steuerpins (Chip Select für linke und rechte Display-Hälfte)
#define LCD_CS1   PC0     // Chip Select 1 (linke Hälfte)
#define LCD_CS2   PC1     // Chip Select 2 (rechte Hälfte)

// LCD-Datenpins (8-bit parallele Datenübertragung)
#define LCD_D0    PC2     // Datenbit 0
#define LCD_D1    PC3     // Datenbit 1
#define LCD_D2    PC4     // Datenbit 2
#define LCD_D3    PC5     // Datenbit 3
#define LCD_D4    PC6     // Datenbit 4
#define LCD_D5    PC7     // Datenbit 5
#define LCD_D6    PB0     // Datenbit 6
#define LCD_D7    PB1     // Datenbit 7

// LCD-Steuerpins
#define LCD_RST   PB6     // Reset-Pin (Display zurücksetzen)
#define LCD_RW    PB7     // Read/Write-Pin (Lesen/Schreiben)
#define LCD_DI    PD0     // Data/Instruction-Pin (Daten/Kommando)
#define LCD_E     PD1     // Enable-Pin (Taktsignal)

// Datenmasken für einfache Pin-Manipulation
#define LCD_DATA_MASK_C   ((1 << LCD_D0) | (1 << LCD_D1) | (1 << LCD_D2) | (1 << LCD_D3) | (1 << LCD_D4) | (1 << LCD_D5))
#define LCD_DATA_MASK_B   ((1 << LCD_D6) | (1 << LCD_D7))

// LCD-Initialisierung
// Konfiguriert alle Pins und initialisiert das Display
void lcd_init(void) {
	// Alle LCD-Pins als Ausgänge konfigurieren
	LCD_DDR |= (1 << LCD_CS1) | (1 << LCD_CS2) | LCD_DATA_MASK_C;  // Port C Pins
	DDRB |= LCD_DATA_MASK_B | (1 << LCD_RST) | (1 << LCD_RW);      // Port B Pins
	DDRD |= (1 << LCD_DI) | (1 << LCD_E);                          // Port D Pins
	
	// Reset-Pin initial auf High setzen
	PORTB |= (1 << LCD_RST);
	
	// Display zurücksetzen (Reset-Pulse)
	PORTB &= ~(1 << LCD_RST);  // Reset auf Low
	_delay_ms(1);              // 1ms warten
	PORTB |= (1 << LCD_RST);   // Reset auf High
	_delay_ms(10);             // 10ms warten für Stabilisierung
	
	// Display-Konfiguration senden
	lcd_write_cmd(0x3F);  // Display ON, Cursor OFF, Blink OFF
	lcd_write_cmd(0x3F);  // Nochmal für Sicherheit
	lcd_write_cmd(0x40);  // Set Display Start Line = 0
	lcd_write_cmd(0xB8);  // Set Page Address = 0
	lcd_write_cmd(0xC0);  // Set Column Address = 0
	
	// Display löschen
	lcd_clear();
}

// LCD-Chip Select aktivieren
// Wählt die entsprechende Display-Hälfte aus
void lcd_select_chip(uint8_t chip) {
	if (chip == 0) {
		// Linke Hälfte aktivieren
		LCD_PORT &= ~(1 << LCD_CS1);  // CS1 auf Low
		LCD_PORT |= (1 << LCD_CS2);   // CS2 auf High (inaktiv)
	} else {
		// Rechte Hälfte aktivieren
		LCD_PORT |= (1 << LCD_CS1);   // CS1 auf High (inaktiv)
		LCD_PORT &= ~(1 << LCD_CS2);  // CS2 auf Low
	}
}

// LCD-Datenbus konfigurieren
// Setzt die Datenpins auf die gewünschten Werte
void lcd_set_data(uint8_t data) {
	// Port C Datenbits (D0-D5) setzen
	LCD_PORT = (LCD_PORT & ~LCD_DATA_MASK_C) | (data & LCD_DATA_MASK_C);
	
	// Port B Datenbits (D6-D7) setzen
	PORTB = (PORTB & ~LCD_DATA_MASK_B) | ((data >> 6) & LCD_DATA_MASK_B);
}

// LCD-Datenbus lesen
// Liest die aktuellen Werte der Datenpins
uint8_t lcd_get_data(void) {
	uint8_t data = 0;
	
	// Port C Datenbits (D0-D5) lesen
	data = (LCD_PIN & LCD_DATA_MASK_C);
	
	// Port B Datenbits (D6-D7) lesen und kombinieren
	data |= ((PINB & LCD_DATA_MASK_B) << 6);
	
	return data;
}

// LCD-Kommando schreiben
// Sendet ein Kommando an das Display
void lcd_write_cmd(uint8_t cmd) {
	// Beide Display-Hälften aktivieren (Kommando geht an beide)
	LCD_PORT &= ~((1 << LCD_CS1) | (1 << LCD_CS2));
	
	// Datenbus als Ausgang konfigurieren
	LCD_DDR |= LCD_DATA_MASK_C;
	DDRB |= LCD_DATA_MASK_B;
	
	// R/W auf Low (Schreiben), DI auf Low (Kommando)
	PORTB &= ~(1 << LCD_RW);
	PORTD &= ~(1 << LCD_DI);
	
	// Daten auf Bus setzen
	lcd_set_data(cmd);
	
	// Enable-Puls (E auf High, dann Low)
	PORTD |= (1 << LCD_E);
	_delay_us(1);  // 1µs warten
	PORTD &= ~(1 << LCD_E);
	
	// Warten bis Display bereit ist
	_delay_us(100);
}

// LCD-Daten schreiben
// Sendet Daten an das Display
void lcd_write_data(uint8_t data) {
	// Datenbus als Ausgang konfigurieren
	LCD_DDR |= LCD_DATA_MASK_C;
	DDRB |= LCD_DATA_MASK_B;
	
	// R/W auf Low (Schreiben), DI auf High (Daten)
	PORTB &= ~(1 << LCD_RW);
	PORTD |= (1 << LCD_DI);
	
	// Daten auf Bus setzen
	lcd_set_data(data);
	
	// Enable-Puls (E auf High, dann Low)
	PORTD |= (1 << LCD_E);
	_delay_us(1);  // 1µs warten
	PORTD &= ~(1 << LCD_E);
	
	// Warten bis Display bereit ist
	_delay_us(100);
}

// LCD-Daten lesen
// Liest Daten vom Display
uint8_t lcd_read_data(void) {
	uint8_t data;
	
	// Datenbus als Eingang konfigurieren
	LCD_DDR &= ~LCD_DATA_MASK_C;
	DDRB &= ~LCD_DATA_MASK_B;
	
	// R/W auf High (Lesen), DI auf High (Daten)
	PORTB |= (1 << LCD_RW);
	PORTD |= (1 << LCD_DI);
	
	// Enable-Puls (E auf High, dann Low)
	PORTD |= (1 << LCD_E);
	_delay_us(1);  // 1µs warten
	PORTD &= ~(1 << LCD_E);
	
	// Daten vom Bus lesen
	data = lcd_get_data();
	
	// Warten bis Display bereit ist
	_delay_us(100);
	
	return data;
}

// LCD-Status lesen
// Prüft ob das Display bereit ist
uint8_t lcd_read_status(void) {
	uint8_t status;
	
	// Datenbus als Eingang konfigurieren
	LCD_DDR &= ~LCD_DATA_MASK_C;
	DDRB &= ~LCD_DATA_MASK_B;
	
	// R/W auf High (Lesen), DI auf Low (Status)
	PORTB |= (1 << LCD_RW);
	PORTD &= ~(1 << LCD_DI);
	
	// Enable-Puls (E auf High, dann Low)
	PORTD |= (1 << LCD_E);
	_delay_us(1);  // 1µs warten
	PORTD &= ~(1 << LCD_E);
	
	// Status vom Bus lesen
	status = lcd_get_data();
	
	// Warten bis Display bereit ist
	_delay_us(100);
	
	return status;
}

// LCD warten bis bereit
// Wartet bis das Display nicht mehr beschäftigt ist
void lcd_wait_ready(void) {
	// Warten bis BUSY-Bit (Bit 7) nicht mehr gesetzt ist
	while (lcd_read_status() & 0x80);
}

// LCD löschen
// Löscht das gesamte Display
void lcd_clear(void) {
	// Alle 8 Seiten durchgehen
	for (uint8_t page = 0; page < 8; page++) {
		// Seitenadresse setzen
		lcd_write_cmd(0xB8 | page);
		
		// Spaltenadresse auf 0 setzen
		lcd_write_cmd(0x40);
		
		// Alle 64 Spalten mit 0 füllen
		for (uint8_t col = 0; col < 64; col++) {
			lcd_write_data(0x00);
		}
	}
}

// LCD-Pixel setzen
// Setzt einen einzelnen Pixel auf Position (x,y)
void lcd_set_pixel(uint8_t x, uint8_t y) {
	uint8_t page = y / 8;      // Seite berechnen (8 Pixel pro Seite)
	uint8_t bit = y % 8;       // Bit-Position in der Seite
	uint8_t data;
	
	// Seitenadresse setzen
	lcd_write_cmd(0xB8 | page);
	
	// Spaltenadresse setzen
	lcd_write_cmd(0x40 | x);
	
	// Aktuelle Daten lesen
	data = lcd_read_data();
	
	// Dummy-Read (Display gibt aktuelle Daten zurück)
	lcd_read_data();
	
	// Bit setzen
	data |= (1 << bit);
	
	// Neue Daten schreiben
	lcd_write_data(data);
}

// LCD-Pixel löschen
// Löscht einen einzelnen Pixel auf Position (x,y)
void lcd_clear_pixel(uint8_t x, uint8_t y) {
	uint8_t page = y / 8;      // Seite berechnen (8 Pixel pro Seite)
	uint8_t bit = y % 8;       // Bit-Position in der Seite
	uint8_t data;
	
	// Seitenadresse setzen
	lcd_write_cmd(0xB8 | page);
	
	// Spaltenadresse setzen
	lcd_write_cmd(0x40 | x);
	
	// Aktuelle Daten lesen
	data = lcd_read_data();
	
	// Dummy-Read (Display gibt aktuelle Daten zurück)
	lcd_read_data();
	
	// Bit löschen
	data &= ~(1 << bit);
	
	// Neue Daten schreiben
	lcd_write_data(data);
}
