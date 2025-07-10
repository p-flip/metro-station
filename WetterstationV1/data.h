/*
 * data.h
 *
 * Datenstrukturen und Konstanten für die Wetterstation
 * Definiert die Strukturen für Sensordaten und Konfiguration
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 

#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>

// EEPROM-Adressen für verschiedene Datentypen
// Diese Adressen definieren wo welche Daten im externen EEPROM gespeichert werden
#define EEPROM_CONFIG_START    0x0000  // Start-Adresse für Konfigurationsdaten
#define EEPROM_DATA_START      0x0100  // Start-Adresse für Sensordaten
#define EEPROM_CALIB_START     0x0F00  // Start-Adresse für Kalibrierungsdaten

// EEPROM-Größen für verschiedene Bereiche
#define EEPROM_CONFIG_SIZE     256     // Größe des Konfigurationsbereichs (256 Bytes)
#define EEPROM_DATA_SIZE       3584    // Größe des Datenbereichs (3.5 KB)
#define EEPROM_CALIB_SIZE      256     // Größe des Kalibrierungsbereichs (256 Bytes)

// Datenstruktur für BME280-Kalibrierungskoeffizienten
// Diese Werte werden vom Sensor gelesen und für die Berechnung der physikalischen Werte benötigt
typedef struct {
	uint16_t dig_T1;  // Temperatur-Kalibrierungskoeffizient 1
	int16_t  dig_T2;  // Temperatur-Kalibrierungskoeffizient 2
	int16_t  dig_T3;  // Temperatur-Kalibrierungskoeffizient 3
	uint16_t dig_P1;  // Druck-Kalibrierungskoeffizient 1
	int16_t  dig_P2;  // Druck-Kalibrierungskoeffizient 2
	int16_t  dig_P3;  // Druck-Kalibrierungskoeffizient 3
	int16_t  dig_P4;  // Druck-Kalibrierungskoeffizient 4
	int16_t  dig_P5;  // Druck-Kalibrierungskoeffizient 5
	int16_t  dig_P6;  // Druck-Kalibrierungskoeffizient 6
	int16_t  dig_P7;  // Druck-Kalibrierungskoeffizient 7
	int16_t  dig_P8;  // Druck-Kalibrierungskoeffizient 8
	int16_t  dig_P9;  // Druck-Kalibrierungskoeffizient 9
	uint8_t  dig_H1;  // Luftfeuchtigkeit-Kalibrierungskoeffizient 1
	int16_t  dig_H2;  // Luftfeuchtigkeit-Kalibrierungskoeffizient 2
	uint8_t  dig_H3;  // Luftfeuchtigkeit-Kalibrierungskoeffizient 3
	int16_t  dig_H4;  // Luftfeuchtigkeit-Kalibrierungskoeffizient 4
	int16_t  dig_H5;  // Luftfeuchtigkeit-Kalibrierungskoeffizient 5
	int8_t   dig_H6;  // Luftfeuchtigkeit-Kalibrierungskoeffizient 6
} bme280_calib_t;

// Datenstruktur für rohe Sensordaten vom BME280
// Diese Werte werden direkt vom Sensor gelesen und müssen noch verarbeitet werden
typedef struct {
	uint32_t adc_T;  // Rohe Temperatur-ADC-Werte (20 Bit)
	uint32_t adc_P;  // Rohe Druck-ADC-Werte (20 Bit)
	uint32_t adc_H;  // Rohe Luftfeuchtigkeit-ADC-Werte (16 Bit)
} bme280_raw_data_t;

// Datenstruktur für verarbeitete Sensordaten
// Diese Werte sind die finalen physikalischen Messwerte
typedef struct {
	int32_t temperature;  // Temperatur in 0.01°C (z.B. 2345 = 23.45°C)
	uint32_t pressure;    // Druck in Pa (z.B. 101325 = 1013.25 hPa)
	uint32_t humidity;    // Luftfeuchtigkeit in 0.01% (z.B. 4567 = 45.67%)
} sensor_data_t;

// Datenstruktur für einen einzelnen Datenpunkt
// Kombiniert Zeitstempel mit Sensordaten
typedef struct {
	uint32_t timestamp;   // Zeitstempel (Sekunden seit Start)
	sensor_data_t data;   // Sensordaten (Temperatur, Druck, Luftfeuchtigkeit)
} data_point_t;

// Datenstruktur für Statistiken über einen Zeitraum
// Berechnete Durchschnitts- und Extremwerte
typedef struct {
	uint32_t start_time;      // Start-Zeitstempel
	uint32_t end_time;        // End-Zeitstempel
	int32_t temp_min;         // Minimale Temperatur
	int32_t temp_max;         // Maximale Temperatur
	int32_t temp_avg;         // Durchschnittliche Temperatur
	uint32_t pressure_min;    // Minimaler Druck
	uint32_t pressure_max;    // Maximaler Druck
	uint32_t pressure_avg;    // Durchschnittlicher Druck
	uint32_t humidity_min;    // Minimale Luftfeuchtigkeit
	uint32_t humidity_max;    // Maximale Luftfeuchtigkeit
	uint32_t humidity_avg;    // Durchschnittliche Luftfeuchtigkeit
	uint16_t data_count;      // Anzahl der Datenpunkte
} statistics_t;

// Datenstruktur für System-Konfiguration
// Einstellungen die im EEPROM gespeichert werden
typedef struct {
	uint16_t magic_number;    // Magic Number zur Erkennung gültiger Konfiguration
	uint16_t version;         // Konfigurationsversion
	uint16_t sample_interval; // Messintervall in Sekunden
	uint16_t display_timeout; // Display-Timeout in Sekunden
	uint8_t  sensor_addr;     // I2C-Adresse des BME280-Sensors
	uint8_t  debug_level;     // Debug-Level (0=off, 1=error, 2=warning, 3=debug)
	uint8_t  reserved[8];     // Reserviert für zukünftige Erweiterungen
} config_t;

// Konstanten für Magic Numbers und Versionen
#define CONFIG_MAGIC_NUMBER   0x5745  // "WE" für Wetterstation
#define CONFIG_VERSION        0x0001  // Version 1.0
#define DEFAULT_SAMPLE_INTERVAL 60    // Standard-Messintervall: 60 Sekunden
#define DEFAULT_DISPLAY_TIMEOUT 30    // Standard-Display-Timeout: 30 Sekunden
#define DEFAULT_SENSOR_ADDR   0x76    // Standard-I2C-Adresse für BME280
#define DEFAULT_DEBUG_LEVEL   1       // Standard-Debug-Level: Error

// Konstanten für Datenverwaltung
#define MAX_DATA_POINTS       512     // Maximale Anzahl gespeicherter Datenpunkte
#define DATA_POINT_SIZE       sizeof(data_point_t)  // Größe eines Datenpunkts
#define STATS_24H_COUNT       24      // Anzahl Stunden für 24h-Statistik
#define STATS_7D_COUNT        168     // Anzahl Stunden für 7-Tage-Statistik

// Konstanten für Sensor-Bereiche
#define TEMP_MIN              -4000   // Minimale Temperatur (-40.00°C)
#define TEMP_MAX              8500    // Maximale Temperatur (85.00°C)
#define PRESSURE_MIN          30000   // Minimaler Druck (300 hPa)
#define PRESSURE_MAX          110000  // Maximaler Druck (1100 hPa)
#define HUMIDITY_MIN          0       // Minimale Luftfeuchtigkeit (0%)
#define HUMIDITY_MAX          10000   // Maximale Luftfeuchtigkeit (100.00%)

// Konstanten für Fehlerbehandlung
#define ERROR_NONE            0       // Kein Fehler
#define ERROR_SENSOR_READ     1       // Fehler beim Sensor-Lesen
#define ERROR_EEPROM_WRITE    2       // Fehler beim EEPROM-Schreiben
#define ERROR_EEPROM_READ     3       // Fehler beim EEPROM-Lesen
#define ERROR_INVALID_DATA    4       // Ungültige Daten
#define ERROR_CONFIG_CORRUPT  5       // Beschädigte Konfiguration

// Funktionen für Datenverwaltung
// Diese Funktionen werden in anderen Modulen implementiert

// Konfiguration laden/speichern
uint8_t config_load(config_t* config);
uint8_t config_save(const config_t* config);
uint8_t config_init(config_t* config);

// Sensordaten verarbeiten
uint8_t sensor_read_raw(bme280_raw_data_t* raw_data);
uint8_t sensor_process_data(const bme280_raw_data_t* raw_data, 
                           const bme280_calib_t* calib, 
                           sensor_data_t* processed_data);

// Datenpunkte speichern/laden
uint8_t data_save_point(const data_point_t* point);
uint8_t data_load_point(uint16_t index, data_point_t* point);
uint16_t data_get_count(void);

// Statistiken berechnen
uint8_t stats_calculate_24h(statistics_t* stats);
uint8_t stats_calculate_7d(statistics_t* stats);

// Kalibrierungsdaten laden
uint8_t calib_load(bme280_calib_t* calib);

#endif /* DATA_H_ */
