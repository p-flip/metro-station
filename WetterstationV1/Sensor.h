/*
 * Sensor.h
 *
 * Header-Datei für BME280-Sensor-Treiber
 * Definiert die Schnittstelle für Temperatur-, Druck- und Luftfeuchtigkeitsmessung
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 

#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdint.h>

// BME280 I2C-Adressen
// Der BME280 kann auf zwei verschiedenen I2C-Adressen betrieben werden
#define BME280_I2C_ADDR_PRIMARY   0x76    // Primäre I2C-Adresse (SDO auf GND)
#define BME280_I2C_ADDR_SECONDARY 0x77    // Sekundäre I2C-Adresse (SDO auf VDD)

// BME280 Register-Adressen
// Diese Register werden für die Konfiguration und Datenauslesung verwendet
#define BME280_REG_TEMP_MSB       0xFA    // Temperatur MSB (Most Significant Byte)
#define BME280_REG_TEMP_LSB       0xFB    // Temperatur LSB (Least Significant Byte)
#define BME280_REG_TEMP_XLSB      0xFC    // Temperatur XLSB (Extra LSB)
#define BME280_REG_PRESS_MSB      0xF7    // Druck MSB
#define BME280_REG_PRESS_LSB      0xF8    // Druck LSB
#define BME280_REG_PRESS_XLSB     0xF9    // Druck XLSB
#define BME280_REG_HUM_MSB        0xFD    // Luftfeuchtigkeit MSB
#define BME280_REG_HUM_LSB        0xFE    // Luftfeuchtigkeit LSB

// BME280 Kalibrierungsregister
// Diese Register enthalten die Kalibrierungskoeffizienten
#define BME280_REG_DIG_T1         0x88    // Temperatur-Kalibrierung 1
#define BME280_REG_DIG_T2         0x8A    // Temperatur-Kalibrierung 2
#define BME280_REG_DIG_T3         0x8C    // Temperatur-Kalibrierung 3
#define BME280_REG_DIG_P1         0x8E    // Druck-Kalibrierung 1
#define BME280_REG_DIG_P2         0x90    // Druck-Kalibrierung 2
#define BME280_REG_DIG_P3         0x92    // Druck-Kalibrierung 3
#define BME280_REG_DIG_P4         0x94    // Druck-Kalibrierung 4
#define BME280_REG_DIG_P5         0x96    // Druck-Kalibrierung 5
#define BME280_REG_DIG_P6         0x98    // Druck-Kalibrierung 6
#define BME280_REG_DIG_P7         0x9A    // Druck-Kalibrierung 7
#define BME280_REG_DIG_P8         0x9C    // Druck-Kalibrierung 8
#define BME280_REG_DIG_P9         0x9E    // Druck-Kalibrierung 9
#define BME280_REG_DIG_H1         0xA1    // Luftfeuchtigkeit-Kalibrierung 1
#define BME280_REG_DIG_H2         0xE1    // Luftfeuchtigkeit-Kalibrierung 2
#define BME280_REG_DIG_H3         0xE3    // Luftfeuchtigkeit-Kalibrierung 3
#define BME280_REG_DIG_H4         0xE4    // Luftfeuchtigkeit-Kalibrierung 4
#define BME280_REG_DIG_H5         0xE5    // Luftfeuchtigkeit-Kalibrierung 5
#define BME280_REG_DIG_H6         0xE7    // Luftfeuchtigkeit-Kalibrierung 6

// BME280 Konfigurationsregister
// Diese Register steuern das Verhalten des Sensors
#define BME280_REG_CTRL_HUM       0xF2    // Luftfeuchtigkeits-Kontrollregister
#define BME280_REG_CTRL_MEAS      0xF4    // Mess-Kontrollregister
#define BME280_REG_CONFIG         0xF5    // Konfigurationsregister
#define BME280_REG_CTRL_MEAS_STATUS 0xF3  // Status-Register für Messungen

// BME280 Chip-ID Register
// Zur Identifikation des Sensors
#define BME280_REG_CHIP_ID        0xD0    // Chip-ID Register (sollte 0x60 sein)
#define BME280_CHIP_ID_VALUE      0x60    // Erwartete Chip-ID für BME280

// BME280 Oversampling-Einstellungen
// Bestimmen die Genauigkeit und Stromverbrauch der Messungen
#define BME280_OVERSAMP_SKIPPED   0x00    // Messung überspringen
#define BME280_OVERSAMP_1X        0x01    // 1x Oversampling
#define BME280_OVERSAMP_2X        0x02    // 2x Oversampling
#define BME280_OVERSAMP_4X        0x03    // 4x Oversampling
#define BME280_OVERSAMP_8X        0x04    // 8x Oversampling
#define BME280_OVERSAMP_16X       0x05    // 16x Oversampling

// BME280 Filter-Einstellungen
// Bestimmen die Filterung der Messwerte
#define BME280_FILTER_OFF         0x00    // Filter aus
#define BME280_FILTER_2           0x01    // 2-Punkt-Filter
#define BME280_FILTER_4           0x02    // 4-Punkt-Filter
#define BME280_FILTER_8           0x03    // 8-Punkt-Filter
#define BME280_FILTER_16          0x04    // 16-Punkt-Filter

// BME280 Standby-Zeit-Einstellungen
// Bestimmen die Zeit zwischen Messungen im Normal-Modus
#define BME280_STANDBY_0_5MS      0x00    // 0.5ms Standby-Zeit
#define BME280_STANDBY_62_5MS     0x01    // 62.5ms Standby-Zeit
#define BME280_STANDBY_125MS      0x02    // 125ms Standby-Zeit
#define BME280_STANDBY_250MS      0x03    // 250ms Standby-Zeit
#define BME280_STANDBY_500MS      0x04    // 500ms Standby-Zeit
#define BME280_STANDBY_1000MS     0x05    // 1000ms Standby-Zeit
#define BME280_STANDBY_10MS       0x06    // 10ms Standby-Zeit
#define BME280_STANDBY_20MS       0x07    // 20ms Standby-Zeit

// BME280 Power-Modus
// Bestimmen den Betriebsmodus des Sensors
#define BME280_SLEEP_MODE         0x00    // Sleep-Modus (niedriger Stromverbrauch)
#define BME280_FORCED_MODE        0x01    // Forced-Modus (einmalige Messung)
#define BME280_NORMAL_MODE        0x03    // Normal-Modus (kontinuierliche Messung)

// BME280 Kalibrierungsdaten-Struktur
// Enthält alle Kalibrierungskoeffizienten für die Berechnung
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
} bme280_calib_data_t;

// BME280 Rohdaten-Struktur
// Enthält die rohen ADC-Werte vom Sensor
typedef struct {
	uint32_t adc_T;  // Rohe Temperatur-ADC-Werte (20 Bit)
	uint32_t adc_P;  // Rohe Druck-ADC-Werte (20 Bit)
	uint32_t adc_H;  // Rohe Luftfeuchtigkeit-ADC-Werte (16 Bit)
} bme280_raw_data_t;

// BME280 verarbeitete Daten-Struktur
// Enthält die finalen physikalischen Messwerte
typedef struct {
	int32_t temperature;  // Temperatur in 0.01°C (z.B. 2345 = 23.45°C)
	uint32_t pressure;    // Druck in Pa (z.B. 101325 = 1013.25 hPa)
	uint32_t humidity;    // Luftfeuchtigkeit in 0.01% (z.B. 4567 = 45.67%)
} bme280_data_t;

// BME280 Konfigurations-Struktur
// Enthält die aktuellen Einstellungen des Sensors
typedef struct {
	uint8_t oversamp_temperature;  // Temperatur-Oversampling
	uint8_t oversamp_pressure;     // Druck-Oversampling
	uint8_t oversamp_humidity;     // Luftfeuchtigkeit-Oversampling
	uint8_t filter;                // Filter-Einstellung
	uint8_t standby_time;          // Standby-Zeit
	uint8_t power_mode;            // Power-Modus
} bme280_config_t;

// Sensor-Initialisierung
// Initialisiert den BME280-Sensor und lädt Kalibrierungsdaten
uint8_t sensor_init(void);

// Sensor-Kalibrierungsdaten laden
// Liest alle Kalibrierungskoeffizienten vom Sensor
uint8_t sensor_load_calibration(bme280_calib_data_t* calib_data);

// Sensor-Konfiguration setzen
// Konfiguriert den Sensor mit den angegebenen Parametern
uint8_t sensor_set_config(const bme280_config_t* config);

// Sensor-Konfiguration laden
// Liest die aktuelle Konfiguration vom Sensor
uint8_t sensor_get_config(bme280_config_t* config);

// Sensor-Rohdaten lesen
// Liest die rohen ADC-Werte vom Sensor
uint8_t sensor_read_raw_data(bme280_raw_data_t* raw_data);

// Sensor-Daten verarbeiten
// Berechnet die physikalischen Werte aus den Rohdaten
void sensor_process_data(const bme280_raw_data_t* raw_data, 
                        const bme280_calib_data_t* calib_data, 
                        bme280_data_t* processed_data);

// Komplette Sensor-Messung
// Führt eine vollständige Messung durch und gibt verarbeitete Daten zurück
uint8_t sensor_read_data(bme280_data_t* data);

// Temperatur messen
// Misst nur die Temperatur
uint8_t sensor_read_temperature(int32_t* temperature);

// Druck messen
// Misst nur den Druck
uint8_t sensor_read_pressure(uint32_t* pressure);

// Luftfeuchtigkeit messen
// Misst nur die Luftfeuchtigkeit
uint8_t sensor_read_humidity(uint32_t* humidity);

// Sensor-Chip-ID prüfen
// Prüft ob der Sensor korrekt antwortet
uint8_t sensor_check_id(void);

// Sensor-Reset durchführen
// Führt einen Software-Reset des Sensors durch
uint8_t sensor_reset(void);

// Sensor-Power-Modus setzen
// Ändert den Power-Modus des Sensors
uint8_t sensor_set_power_mode(uint8_t mode);

// Sensor-Oversampling setzen
// Ändert die Oversampling-Einstellungen
uint8_t sensor_set_oversampling(uint8_t temp_oversamp, uint8_t press_oversamp, uint8_t hum_oversamp);

// Sensor-Filter setzen
// Ändert die Filter-Einstellung
uint8_t sensor_set_filter(uint8_t filter);

// Sensor-Standby-Zeit setzen
// Ändert die Standby-Zeit im Normal-Modus
uint8_t sensor_set_standby_time(uint8_t standby_time);

// Sensor-Status prüfen
// Prüft ob der Sensor bereit ist
uint8_t sensor_is_ready(void);

// Sensor-Statistiken
// Gibt Informationen über Sensor-Nutzung zurück
typedef struct {
	uint16_t measurements;        // Anzahl durchgeführter Messungen
	uint16_t errors;              // Anzahl aufgetretener Fehler
	uint16_t calibration_errors;  // Anzahl Kalibrierungsfehler
	uint8_t  is_initialized;      // Initialisierungsstatus
	uint8_t  is_ready;            // Bereitschaftsstatus
} sensor_stats_t;

// Sensor-Statistiken abrufen
// Liest aktuelle Sensor-Statistiken
void sensor_get_stats(sensor_stats_t* stats);

// Sensor-Statistiken zurücksetzen
// Setzt alle Zähler zurück
void sensor_reset_stats(void);

// Sensor-Selbsttest
// Führt einen Selbsttest des Sensors durch
uint8_t sensor_self_test(void);

// Sensor-Temperatur-Kompensation
// Kompensiert Temperatur-Einflüsse auf andere Messungen
int32_t sensor_compensate_temperature(int32_t adc_T, bme280_calib_data_t* calib_data);

// Sensor-Druck-Kompensation
// Kompensiert Druck-Messungen basierend auf Temperatur
uint32_t sensor_compensate_pressure(int32_t adc_P, int32_t t_fine, bme280_calib_data_t* calib_data);

// Sensor-Luftfeuchtigkeit-Kompensation
// Kompensiert Luftfeuchtigkeit-Messungen basierend auf Temperatur
uint32_t sensor_compensate_humidity(int32_t adc_H, int32_t t_fine, bme280_calib_data_t* calib_data);

#endif /* SENSOR_H_ */