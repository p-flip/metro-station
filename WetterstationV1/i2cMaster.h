/*
 * i2cMaster.h
 *
 * Header-Datei für I2C-Master-Treiber
 * Definiert die Schnittstelle für Software-I2C-Kommunikation mit BME280-Sensor
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 

#ifndef I2CMASTER_H_
#define I2CMASTER_H_

#include <stdint.h>

// I2C-Konfiguration für BME280-Sensor
// Diese Werte bestimmen die I2C-Kommunikation
#define I2C_FREQUENCY      100000  // I2C-Frequenz in Hz (100 kHz)
#define I2C_TIMEOUT_MS     100     // Timeout für I2C-Operationen in ms
#define I2C_RETRY_COUNT    3       // Anzahl Wiederholungsversuche bei Fehlern

// I2C-Geräteadressen
// Standard-I2C-Adressen für verschiedene Sensoren
#define BME280_I2C_ADDR    0x76    // BME280 I2C-Adresse (Alternative: 0x77)
#define BMP280_I2C_ADDR    0x76    // BMP280 I2C-Adresse (Alternative: 0x77)
#define SHT30_I2C_ADDR     0x44    // SHT30 I2C-Adresse
#define MPU6050_I2C_ADDR   0x68    // MPU6050 I2C-Adresse

// I2C-Status-Codes
// Rückgabewerte für I2C-Operationen
#define I2C_OK             0       // Operation erfolgreich
#define I2C_ERROR_START    1       // Start-Bedingung fehlgeschlagen
#define I2C_ERROR_ADDR     2       // Adressierung fehlgeschlagen
#define I2C_ERROR_DATA     3       // Datenübertragung fehlgeschlagen
#define I2C_ERROR_STOP     4       // Stop-Bedingung fehlgeschlagen
#define I2C_ERROR_TIMEOUT  5       // Timeout aufgetreten
#define I2C_ERROR_NACK     6       // NACK empfangen

// I2C-Initialisierung
// Konfiguriert die I2C-Pins als Open-Drain-Ausgänge
void i2c_init(void);

// I2C-Start-Bedingung erzeugen
// SDA fällt während SCL High ist
void i2c_start(void);

// I2C-Stop-Bedingung erzeugen
// SDA steigt während SCL High ist
void i2c_stop(void);

// I2C-Bit senden
// Sendet ein einzelnes Bit über die I2C-Leitung
void i2c_write_bit(uint8_t bit);

// I2C-Bit lesen
// Liest ein einzelnes Bit von der I2C-Leitung
uint8_t i2c_read_bit(void);

// I2C-Byte senden
// Sendet ein 8-bit Byte über I2C
uint8_t i2c_write_byte(uint8_t data);

// I2C-Byte lesen
// Liest ein 8-bit Byte über I2C
uint8_t i2c_read_byte(uint8_t ack);

// I2C-Byte an Register schreiben
// Schreibt ein Byte an eine spezifische Register-Adresse eines I2C-Geräts
uint8_t i2c_write_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t data);

// I2C-Byte von Register lesen
// Liest ein Byte von einer spezifischen Register-Adresse eines I2C-Geräts
uint8_t i2c_read_reg(uint8_t device_addr, uint8_t reg_addr);

// Mehrere Bytes von Register lesen
// Liest mehrere aufeinanderfolgende Bytes von einem I2C-Gerät
void i2c_read_regs(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint8_t length);

// I2C-Gerät auf Bus suchen
// Prüft ob ein I2C-Gerät mit der angegebenen Adresse antwortet
uint8_t i2c_scan_device(uint8_t device_addr);

// I2C-Bus scannen
// Sucht nach allen I2C-Geräten auf dem Bus
void i2c_scan_bus(uint8_t* device_list, uint8_t* device_count);

// I2C-Reset durchführen
// Führt einen Software-Reset des I2C-Busses durch
void i2c_reset(void);

// I2C-Bus freigeben
// Gibt den I2C-Bus frei (Stop-Bedingung senden)
void i2c_release(void);

// I2C-Timeout setzen
// Setzt den Timeout für I2C-Operationen
void i2c_set_timeout(uint16_t timeout_ms);

// I2C-Frequenz einstellen
// Passt die I2C-Frequenz an (falls unterstützt)
void i2c_set_frequency(uint32_t frequency_hz);

// I2C-Pull-up-Widerstände aktivieren/deaktivieren
// Kontrolliert die internen Pull-up-Widerstände
void i2c_set_pullups(uint8_t enable);

// I2C-Statistiken
// Gibt Informationen über I2C-Nutzung zurück
typedef struct {
	uint16_t transactions;      // Anzahl I2C-Transaktionen
	uint16_t bytes_sent;        // Anzahl gesendeter Bytes
	uint16_t bytes_received;    // Anzahl empfangener Bytes
	uint16_t errors;            // Anzahl aufgetretener Fehler
	uint16_t timeouts;          // Anzahl Timeouts
	uint16_t nacks;             // Anzahl NACKs
	uint8_t  devices_found;     // Anzahl gefundener Geräte
} i2c_stats_t;

// I2C-Statistiken abrufen
// Liest aktuelle I2C-Statistiken
void i2c_get_stats(i2c_stats_t* stats);

// I2C-Statistiken zurücksetzen
// Setzt alle Zähler zurück
void i2c_reset_stats(void);

// I2C-Bus-Status prüfen
// Prüft ob der I2C-Bus frei ist
uint8_t i2c_bus_free(void);

// I2C-Bus-Arbitration
// Behandelt Bus-Arbitration bei Konflikten
uint8_t i2c_arbitration_lost(void);

// I2C-Clock-Stretching
// Behandelt Clock-Stretching von Slave-Geräten
void i2c_handle_clock_stretch(void);

// I2C-Multi-Master-Support
// Funktionen für Multi-Master-I2C-Busse
uint8_t i2c_master_mode(void);
void i2c_set_master_mode(uint8_t enable);

// I2C-Debug-Funktionen
// Hilfsfunktionen für Debugging
void i2c_debug_enable(uint8_t enable);
void i2c_print_status(void);
void i2c_print_stats(void);

// I2C-Interrupt-Handler (falls verwendet)
// Wird automatisch aufgerufen bei I2C-Ereignissen
// Diese Funktionen sind in der .c-Datei implementiert
// void i2c_interrupt_handler(void);

#endif /* I2CMASTER_H_ */
