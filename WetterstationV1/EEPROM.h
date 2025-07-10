/*
 * EEPROM.h
 *
 * Header-Datei für externes EEPROM-Treiber
 * Definiert die Schnittstelle für SPI-basierte EEPROM-Kommunikation
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 

#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

// EEPROM-Kommandos (laut Datenblatt des verwendeten EEPROMs)
// Diese Kommandos werden über SPI an das EEPROM gesendet
#define EEPROM_CMD_READ   0x03  // Read Data from Memory Array
#define EEPROM_CMD_WRITE  0x02  // Write Data to Memory Array
#define EEPROM_CMD_WREN   0x06  // Write Enable (vor jedem Schreibvorgang nötig)
#define EEPROM_CMD_RDSR   0x05  // Read Status Register

// EEPROM-Status-Bits
// Diese Bits werden im Status-Register verwendet
#define EEPROM_STATUS_WIP 0x01  // Write In Progress (Schreibvorgang läuft)
#define EEPROM_STATUS_WEL 0x02  // Write Enable Latch (Schreiben erlaubt)

// EEPROM-Timing-Konstanten
// Diese Werte bestimmen die Wartezeiten für EEPROM-Operationen
#define EEPROM_WRITE_DELAY_MS  10   // Wartezeit nach Schreibvorgang (ms)
#define EEPROM_PAGE_SIZE       64   // Größe einer EEPROM-Seite (Bytes)
#define EEPROM_MAX_ADDRESS     0x7FFF // Maximale Adresse (32KB EEPROM)

// SPI-Initialisierung für EEPROM
// Konfiguriert die SPI-Hardware für EEPROM-Kommunikation
void spi_init(void);

// SPI-Datentransfer
// Sendet und empfängt ein Byte über SPI
uint8_t spi_transfer(uint8_t data);

// EEPROM-Chip Select aktivieren
// Aktiviert das EEPROM für Kommunikation
void eeprom_select(void);

// EEPROM-Chip Select deaktivieren
// Deaktiviert das EEPROM und aktiviert andere SPI-Geräte
void eeprom_deselect(void);

// Write Enable für EEPROM senden
// Muss vor jedem Schreibvorgang aufgerufen werden
void eeprom_write_enable(void);

// Ein Byte an spezifische Adresse schreiben
// Schreibt ein Byte an die angegebene EEPROM-Adresse
void eeprom_write_byte(uint16_t address, uint8_t data);

// Ein Byte von spezifischer Adresse lesen
// Liest ein Byte von der angegebenen EEPROM-Adresse
uint8_t eeprom_read_byte(uint16_t address);

// Status-Register des EEPROMs lesen
// Liest das Status-Register um den aktuellen Zustand zu prüfen
uint8_t eeprom_read_status(void);

// Warten bis EEPROM bereit ist
// Wartet bis ein Schreibvorgang abgeschlossen ist
void eeprom_wait_until_ready(void);

// Mehrere Bytes in EEPROM schreiben
// Schreibt ein Array von Bytes ab der angegebenen Adresse
void eeprom_write_block(uint16_t address, const uint8_t* data, uint16_t length);

// Mehrere Bytes aus EEPROM lesen
// Liest ein Array von Bytes ab der angegebenen Adresse
void eeprom_read_block(uint16_t address, uint8_t* data, uint16_t length);

// EEPROM-Seite schreiben
// Schreibt eine komplette EEPROM-Seite (64 Bytes)
void eeprom_write_page(uint16_t page_address, const uint8_t* data);

// EEPROM-Seite lesen
// Liest eine komplette EEPROM-Seite (64 Bytes)
void eeprom_read_page(uint16_t page_address, uint8_t* data);

// EEPROM-Bereich löschen
// Setzt einen Bereich des EEPROMs auf 0xFF
void eeprom_erase_block(uint16_t start_address, uint16_t length);

// EEPROM-Gerät testen
// Führt einen Selbsttest des EEPROMs durch
uint8_t eeprom_test(void);

// EEPROM-Informationen lesen
// Liest Geräte-Informationen aus dem EEPROM
void eeprom_get_info(uint8_t* manufacturer_id, uint8_t* device_id);

// EEPROM-Schreibschutz aktivieren/deaktivieren
// Kontrolliert den Hardware-Schreibschutz (falls verfügbar)
void eeprom_write_protect(uint8_t enable);

// EEPROM-Speicherplatz prüfen
// Prüft ob genügend freier Speicherplatz verfügbar ist
uint8_t eeprom_check_space(uint16_t required_bytes);

// EEPROM-Daten integrität prüfen
// Berechnet und prüft Checksummen für Datenintegrität
uint16_t eeprom_calculate_checksum(uint16_t start_address, uint16_t length);
uint8_t eeprom_verify_checksum(uint16_t start_address, uint16_t length, uint16_t expected_checksum);

// EEPROM-Backup erstellen
// Erstellt eine Sicherungskopie eines EEPROM-Bereichs
void eeprom_backup(uint16_t source_address, uint16_t dest_address, uint16_t length);

// EEPROM-Restore durchführen
// Stellt Daten aus einer Sicherungskopie wieder her
void eeprom_restore(uint16_t source_address, uint16_t dest_address, uint16_t length);

// EEPROM-Formatierung
// Formatiert das gesamte EEPROM (setzt alle Bytes auf 0xFF)
void eeprom_format(void);

// EEPROM-Statistiken
// Gibt Informationen über EEPROM-Nutzung zurück
typedef struct {
	uint16_t total_bytes;      // Gesamte EEPROM-Größe
	uint16_t used_bytes;       // Verwendete Bytes
	uint16_t free_bytes;       // Freie Bytes
	uint16_t write_cycles;     // Anzahl Schreibzyklen
	uint16_t read_cycles;      // Anzahl Lesezyklen
	uint16_t error_count;      // Anzahl aufgetretener Fehler
} eeprom_stats_t;

// EEPROM-Statistiken abrufen
// Liest aktuelle Nutzungsstatistiken
void eeprom_get_stats(eeprom_stats_t* stats);

// EEPROM-Statistiken zurücksetzen
// Setzt alle Zähler zurück
void eeprom_reset_stats(void);

#endif /* EEPROM_H_ */