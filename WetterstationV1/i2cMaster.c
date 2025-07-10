/*
 * i2cMaster.c
 *
 * I2C-Master-Treiber für ATmega8
 * Implementiert die I2C-Kommunikation mit dem BME280-Sensor
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 
//#define F_CPU 3686400UL

#include <avr/io.h>
#include <util/delay.h>
#include "i2cMaster.h"

// I2C-Pin-Definitionen für ATmega8
// Diese Pins werden für die I2C-Kommunikation verwendet
#define I2C_DDR   DDRC    // Data Direction Register für I2C-Pins
#define I2C_PORT  PORTC   // Port Register für I2C-Pins
#define I2C_PIN   PINC    // Pin Register für I2C-Pins (zum Lesen)

// I2C-Pins (Software-I2C-Implementierung)
#define I2C_SDA   PC4     // Serial Data Line
#define I2C_SCL   PC5     // Serial Clock Line

// I2C-Timing-Definitionen
// Diese Werte bestimmen die Geschwindigkeit der I2C-Kommunikation
#define I2C_DELAY 5       // Verzögerung in Mikrosekunden für Timing

// I2C-Initialisierung
// Konfiguriert die I2C-Pins als Open-Drain-Ausgänge
void i2c_init(void) {
	// I2C-Pins als Ausgänge konfigurieren
	I2C_DDR |= (1 << I2C_SDA) | (1 << I2C_SCL);
	
	// Beide Pins auf High setzen (inaktiver Zustand)
	I2C_PORT |= (1 << I2C_SDA) | (1 << I2C_SCL);
	
	// Kurze Verzögerung für Stabilisierung
	_delay_us(10);
}

// I2C-Start-Bedingung erzeugen
// SDA fällt während SCL High ist
void i2c_start(void) {
	// SDA und SCL auf High setzen
	I2C_PORT |= (1 << I2C_SDA) | (1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SDA auf Low setzen (Start-Bedingung beginnt)
	I2C_PORT &= ~(1 << I2C_SDA);
	_delay_us(I2C_DELAY);
	
	// SCL auf Low setzen (Start-Bedingung abgeschlossen)
	I2C_PORT &= ~(1 << I2C_SCL);
	_delay_us(I2C_DELAY);
}

// I2C-Stop-Bedingung erzeugen
// SDA steigt während SCL High ist
void i2c_stop(void) {
	// SCL auf Low setzen
	I2C_PORT &= ~(1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SDA auf Low setzen
	I2C_PORT &= ~(1 << I2C_SDA);
	_delay_us(I2C_DELAY);
	
	// SCL auf High setzen
	I2C_PORT |= (1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SDA auf High setzen (Stop-Bedingung)
	I2C_PORT |= (1 << I2C_SDA);
	_delay_us(I2C_DELAY);
}

// I2C-Bit senden
// Sendet ein einzelnes Bit über die I2C-Leitung
void i2c_write_bit(uint8_t bit) {
	// SCL auf Low setzen
	I2C_PORT &= ~(1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SDA entsprechend dem Bit setzen
	if (bit) {
		I2C_PORT |= (1 << I2C_SDA);   // Bit = 1
	} else {
		I2C_PORT &= ~(1 << I2C_SDA);  // Bit = 0
	}
	_delay_us(I2C_DELAY);
	
	// SCL auf High setzen (Bit wird übertragen)
	I2C_PORT |= (1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SCL auf Low setzen
	I2C_PORT &= ~(1 << I2C_SCL);
	_delay_us(I2C_DELAY);
}

// I2C-Bit lesen
// Liest ein einzelnes Bit von der I2C-Leitung
uint8_t i2c_read_bit(void) {
	uint8_t bit;
	
	// SCL auf Low setzen
	I2C_PORT &= ~(1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SDA als Eingang konfigurieren (High-Zustand)
	I2C_DDR &= ~(1 << I2C_SDA);
	I2C_PORT |= (1 << I2C_SDA);  // Pull-up aktivieren
	_delay_us(I2C_DELAY);
	
	// SCL auf High setzen (Bit wird gelesen)
	I2C_PORT |= (1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SDA-Pin lesen
	bit = (I2C_PIN & (1 << I2C_SDA)) ? 1 : 0;
	
	// SCL auf Low setzen
	I2C_PORT &= ~(1 << I2C_SCL);
	_delay_us(I2C_DELAY);
	
	// SDA wieder als Ausgang konfigurieren
	I2C_DDR |= (1 << I2C_SDA);
	
	return bit;  // Gelesenes Bit zurückgeben
}

// I2C-Byte senden
// Sendet ein 8-bit Byte über I2C
uint8_t i2c_write_byte(uint8_t data) {
	uint8_t ack;
	
	// 8 Bits senden (MSB zuerst)
	for (int8_t i = 7; i >= 0; i--) {
		i2c_write_bit((data >> i) & 0x01);  // Einzelnes Bit senden
	}
	
	// ACK-Bit lesen (9. Bit)
	ack = i2c_read_bit();
	
	return ack;  // ACK-Status zurückgeben (0 = ACK, 1 = NACK)
}

// I2C-Byte lesen
// Liest ein 8-bit Byte über I2C
uint8_t i2c_read_byte(uint8_t ack) {
	uint8_t data = 0;
	
	// 8 Bits lesen (MSB zuerst)
	for (int8_t i = 7; i >= 0; i--) {
		uint8_t bit = i2c_read_bit();  // Einzelnes Bit lesen
		data |= (bit << i);            // Bit an richtige Position setzen
	}
	
	// ACK/NACK senden (9. Bit)
	i2c_write_bit(ack);
	
	return data;  // Gelesenes Byte zurückgeben
}

// I2C-Byte an Register schreiben
// Schreibt ein Byte an eine spezifische Register-Adresse eines I2C-Geräts
uint8_t i2c_write_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t data) {
	uint8_t status;
	
	i2c_start();                    // Start-Bedingung senden
	
	// Geräteadresse mit Write-Bit senden
	status = i2c_write_byte((device_addr << 1) | 0);
	if (status) {                   // NACK empfangen
		i2c_stop();
		return 1;                   // Fehler zurückgeben
	}
	
	// Register-Adresse senden
	status = i2c_write_byte(reg_addr);
	if (status) {                   // NACK empfangen
		i2c_stop();
		return 2;                   // Fehler zurückgeben
	}
	
	// Daten senden
	status = i2c_write_byte(data);
	if (status) {                   // NACK empfangen
		i2c_stop();
		return 3;                   // Fehler zurückgeben
	}
	
	i2c_stop();                     // Stop-Bedingung senden
	return 0;                       // Erfolg zurückgeben
}

// I2C-Byte von Register lesen
// Liest ein Byte von einer spezifischen Register-Adresse eines I2C-Geräts
uint8_t i2c_read_reg(uint8_t device_addr, uint8_t reg_addr) {
	uint8_t data;
	
	i2c_start();                    // Start-Bedingung senden
	
	// Geräteadresse mit Write-Bit senden (für Register-Adresse)
	i2c_write_byte((device_addr << 1) | 0);
	
	// Register-Adresse senden
	i2c_write_byte(reg_addr);
	
	// Wiederholte Start-Bedingung für Lese-Operation
	i2c_start();
	
	// Geräteadresse mit Read-Bit senden
	i2c_write_byte((device_addr << 1) | 1);
	
	// Daten lesen (mit NACK für letztes Byte)
	data = i2c_read_byte(1);        // NACK senden
	
	i2c_stop();                     // Stop-Bedingung senden
	return data;                    // Gelesenes Byte zurückgeben
}

// Mehrere Bytes von Register lesen
// Liest mehrere aufeinanderfolgende Bytes von einem I2C-Gerät
void i2c_read_regs(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint8_t length) {
	i2c_start();                    // Start-Bedingung senden
	
	// Geräteadresse mit Write-Bit senden (für Register-Adresse)
	i2c_write_byte((device_addr << 1) | 0);
	
	// Register-Adresse senden
	i2c_write_byte(reg_addr);
	
	// Wiederholte Start-Bedingung für Lese-Operation
	i2c_start();
	
	// Geräteadresse mit Read-Bit senden
	i2c_write_byte((device_addr << 1) | 1);
	
	// Mehrere Bytes lesen
	for (uint8_t i = 0; i < length; i++) {
		uint8_t ack = (i == length - 1) ? 1 : 0;  // NACK nur für letztes Byte
		data[i] = i2c_read_byte(ack);
	}
	
	i2c_stop();                     // Stop-Bedingung senden
}

// I2C-Gerät auf Bus suchen
// Prüft ob ein I2C-Gerät mit der angegebenen Adresse antwortet
uint8_t i2c_scan_device(uint8_t device_addr) {
	uint8_t status;
	
	i2c_start();                    // Start-Bedingung senden
	
	// Geräteadresse mit Write-Bit senden
	status = i2c_write_byte((device_addr << 1) | 0);
	
	i2c_stop();                     // Stop-Bedingung senden
	
	return !status;                 // 1 wenn Gerät gefunden, 0 wenn nicht
}
