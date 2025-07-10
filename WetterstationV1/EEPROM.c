/*
 * EEPROM.c
 *
 * SPI-Treiber für externes EEPROM (z.B. 25LC512 oder ähnlich)
 * Implementiert die SPI-Kommunikation für Datenspeicherung
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 
//#define F_CPU 3686400UL

#include <avr/io.h>
#include <util/delay.h>
#include "EEPROM.h"

// SPI Pin-Definitionen für ATmega8
// Diese Pins werden für die SPI-Kommunikation mit dem EEPROM verwendet
#define SPI_DDR   DDRB    // Data Direction Register für SPI-Pins
#define SPI_PORT  PORTB   // Port Register für SPI-Pins
#define SPI_MISO  PB4     // Master In Slave Out (Daten vom EEPROM)
#define SPI_MOSI  PB3     // Master Out Slave In (Daten zum EEPROM)
#define SPI_SCK   PB5     // Serial Clock (Taktsignal)
#define SPI_CS    PB2     // Chip Select (EEPROM aktivieren/deaktivieren)

// EEPROM-Kommandos (laut Datenblatt des verwendeten EEPROMs)
#define EEPROM_CMD_READ   0x03  // Read Data from Memory Array
#define EEPROM_CMD_WRITE  0x02  // Write Data to Memory Array
#define EEPROM_CMD_WREN   0x06  // Write Enable (vor jedem Schreibvorgang nötig)
#define EEPROM_CMD_RDSR   0x05  // Read Status Register

// SPI-Initialisierung für EEPROM-Kommunikation
void spi_init(void) {
	// SPI-Pins als Ausgänge konfigurieren (außer MISO)
	SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS);
	SPI_DDR &= ~(1 << SPI_MISO);  // MISO als Eingang
	
	// SPI-Register konfigurieren
	// SPE = SPI Enable, MSTR = Master Mode, SPR0 = Prescaler /16
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

// SPI-Datentransfer (sendet und empfängt ein Byte)
uint8_t spi_transfer(uint8_t data) {
	SPDR = data;  // Daten in SPI Data Register schreiben
	
	// Warten bis Transfer abgeschlossen ist (SPIF-Bit wird gesetzt)
	while (!(SPSR & (1 << SPIF)));
	
	return SPDR;  // Empfangene Daten zurückgeben
}

// EEPROM Chip Select aktivieren (Low-Aktiv)
void eeprom_select(void) {
	SPI_PORT &= ~(1 << SPI_CS);  // CS auf Low setzen (EEPROM aktivieren)
	SPI_DDR &= ~(1 << SPI_MISO); // MISO als Input definieren (für EEPROM-Daten)
	
	// Alle anderen CS-Pins inaktiv setzen (Display hört nicht auf E)
	// Dies verhindert Konflikte mit anderen SPI-Geräten
	PORTC |= _BV(PC1) | _BV(PC0);
}

// EEPROM Chip Select deaktivieren (High-Aktiv)
void eeprom_deselect(void) {
	SPI_PORT |= (1 << SPI_CS);   // CS auf High setzen (EEPROM deaktivieren)
	SPI_PORT &= ~(1 << SPI_MISO); // MISO-Pin zurücksetzen
	SPI_DDR  |=  (1 << SPI_MISO); // MISO als Output definieren (für Display)
	
	// Display-CS-Pins aktivieren
	PORTC &= ~(_BV(PC1) | _BV(PC0));
	// Nun kannst du E per GPIO pulsen und das Display updated
}

// Write Enable für EEPROM senden
// Muss vor jedem Schreibvorgang aufgerufen werden
void eeprom_write_enable(void) {
	eeprom_select();                    // EEPROM aktivieren
	spi_transfer(EEPROM_CMD_WREN);      // Write Enable Kommando senden
	eeprom_deselect();                  // EEPROM deaktivieren
}

// Ein Byte an eine spezifische Adresse im EEPROM schreiben
void eeprom_write_byte(uint16_t address, uint8_t data) {
	eeprom_write_enable();              // Write Enable senden
	
	eeprom_select();                    // EEPROM aktivieren
	spi_transfer(EEPROM_CMD_WRITE);     // Write-Kommando senden
	spi_transfer((uint8_t)(address >> 8));   // High-Byte der Adresse
	spi_transfer((uint8_t)(address & 0xFF)); // Low-Byte der Adresse
	spi_transfer(data);                 // Daten-Byte senden
	eeprom_deselect();                  // EEPROM deaktivieren
	
	_delay_ms(10);  // Warten bis Schreibvorgang abgeschlossen ist (typ. 5 ms)
}

// Ein Byte von einer spezifischen Adresse im EEPROM lesen
uint8_t eeprom_read_byte(uint16_t address) {
	eeprom_select();                    // EEPROM aktivieren
	spi_transfer(EEPROM_CMD_READ);      // Read-Kommando senden
	spi_transfer((uint8_t)(address >> 8));   // High-Byte der Adresse
	spi_transfer((uint8_t)(address & 0xFF)); // Low-Byte der Adresse
	uint8_t data = spi_transfer(0x00);  // Dummy-Byte senden, Daten empfangen
	eeprom_deselect();                  // EEPROM deaktivieren
	
	return data;  // Gelesenes Byte zurückgeben
}

// Status-Register des EEPROMs lesen
// WIP-Bit zeigt an, ob ein Schreibvorgang läuft
uint8_t eeprom_read_status(void) {
	eeprom_select();                    // EEPROM aktivieren
	spi_transfer(EEPROM_CMD_RDSR);      // Read Status Register Kommando
	uint8_t status = spi_transfer(0x00); // Status-Byte empfangen
	eeprom_deselect();                  // EEPROM deaktivieren
	
	return status;  // Status-Byte zurückgeben
}

// Warten bis EEPROM bereit ist (Schreibvorgang abgeschlossen)
void eeprom_wait_until_ready(void) {
	// WIP-Bit (Write In Progress) prüfen - Bit 0 im Status-Register
	while (eeprom_read_status() & 0x01);
}

// Mehrere Bytes in das EEPROM schreiben
// Schreibt Byte für Byte und wartet nach jedem Byte
void eeprom_write_block(uint16_t address, const uint8_t* data, uint16_t length) {
	for (uint16_t i = 0; i < length; i++) {
		eeprom_write_byte(address + i, data[i]);  // Ein Byte schreiben
		eeprom_wait_until_ready();  // Warten bis Schreibvorgang abgeschlossen
	}
}

// Mehrere Bytes aus dem EEPROM lesen
// Liest Byte für Byte ohne Wartezeiten
void eeprom_read_block(uint16_t address, uint8_t* data, uint16_t length) {
	for (uint16_t i = 0; i < length; i++) {
		data[i] = eeprom_read_byte(address + i);  // Ein Byte lesen
	}
}