/*
 * EEPROM.c
 *
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 
//#define F_CPU 3686400UL


#include <avr/io.h>
#include <util/delay.h>
#include "EEPROM.h"




// SPI Pins
#define SPI_DDR   DDRB
#define SPI_PORT  PORTB
#define SPI_MISO  PB4
#define SPI_MOSI  PB3
#define SPI_SCK   PB5
#define SPI_CS    PB2

// EEPROM Kommandos
#define EEPROM_CMD_READ   0x03
#define EEPROM_CMD_WRITE  0x02
#define EEPROM_CMD_WREN   0x06
#define EEPROM_CMD_RDSR   0x05


// SPI Initialisierung
void spi_init(void) {
	SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS);
	SPI_DDR &= ~(1 << SPI_MISO);
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); // clk/16
}
// SPI Transfer

uint8_t spi_transfer(uint8_t data) {
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}


// EEPROM CS Low
void eeprom_select(void) {
	SPI_PORT &= ~(1 << SPI_CS);
	SPI_DDR &= ~(1 << SPI_MISO); //Port als Input definiert
	// Alle CS inaktiv ? Display hört nicht auf E
	PORTC |= _BV(PC1) | _BV(PC0);
}

// EEPROM CS High
void eeprom_deselect(void) {
	SPI_PORT |= (1 << SPI_CS);
	SPI_PORT &= ~(1 << SPI_MISO);
	SPI_DDR  |=  (1 << SPI_MISO); //Port als Output definieren (für Display)
	PORTC &= ~(_BV(PC1) | _BV(PC0));
	// nun kannst du E per GPIO pulsen und das Display updated
}
// EEPROM Write Enable
void eeprom_write_enable(void) {
	eeprom_select();
	spi_transfer(EEPROM_CMD_WREN);
	eeprom_deselect();
}
// EEPROM Write Byte (1 Byte bei Adresse)
void eeprom_write_byte(uint16_t address, uint8_t data) {
	eeprom_write_enable();
	eeprom_select();
	spi_transfer(EEPROM_CMD_WRITE);
	spi_transfer((uint8_t)(address >> 8));   // MSB
	spi_transfer((uint8_t)(address & 0xFF)); // LSB
	spi_transfer(data);
	eeprom_deselect();
	_delay_ms(10); // Zeit zum Schreiben (typ. 5 ms)
}

// EEPROM Read Byte
uint8_t eeprom_read_byte(uint16_t address) {
	eeprom_select();
	spi_transfer(EEPROM_CMD_READ);
	spi_transfer((uint8_t)(address >> 8));   // MSB
	spi_transfer((uint8_t)(address & 0xFF)); // LSB
	uint8_t data = spi_transfer(0x00);       // Dummy senden, Byte lesen
	eeprom_deselect();
	return data;
}

// EEPROM Status lesen
uint8_t eeprom_read_status(void) {
	eeprom_select();
	spi_transfer(EEPROM_CMD_RDSR);
	uint8_t status = spi_transfer(0x00);
	eeprom_deselect();
	return status;
}
// Warten bis EEPROM ready ist
void eeprom_wait_until_ready(void) {
	while (eeprom_read_status() & 0x01); // WIP-Bit prüfen
}

// Mehrere Bytes schreiben
void eeprom_write_block(uint16_t address, const uint8_t* data, uint16_t length) {
	for (uint16_t i = 0; i < length; i++) {
		eeprom_write_byte(address + i, data[i]);
		eeprom_wait_until_ready();  // Optional: Für Sicherheit nach jedem Byte warten
	}
}

// Mehrere Bytes lesen
void eeprom_read_block(uint16_t address, uint8_t* data, uint16_t length) {
	for (uint16_t i = 0; i < length; i++) {
		data[i] = eeprom_read_byte(address + i);
	}
}