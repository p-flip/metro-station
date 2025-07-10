/*
 * Sensor.c
 *
 * BME280 Sensor-Treiber für Temperatur- und Druckmessung
 * Implementiert die I2C-Kommunikation mit dem BME280 Sensor
 * 
 * Created: 25.05.2025 13:53:11
 *  Author: morri
 */ 

#include "Sensor.h"
#include "i2cMaster.h"

// BME280 I2C-Adresse (0x76 = Standard-Adresse)
#define BME280_ADDR 0x76

// BME280 Register-Adressen (laut Datenblatt)
#define REG_CTRL_MEAS 0xF4  // Control Measurement Register
#define REG_CONFIG 0xF5     // Configuration Register  
#define REG_DATA 0xF7       // Data Register (Startadresse für Messdaten)

// Kalibrierungsvariablen (laut BME280-Datenblatt)
// Diese Werte werden beim Start vom Sensor gelesen
// Temperatur-Kalibrierwerte (für Temperaturkompensation)
uint16_t dig_T1;  // Temperatur-Kalibrierung 1
int16_t  dig_T2;  // Temperatur-Kalibrierung 2
int16_t  dig_T3;  // Temperatur-Kalibrierung 3

// Druck-Kalibrierwerte (für Druckkompensation)
uint16_t dig_P1;  // Druck-Kalibrierung 1
int16_t  dig_P2;  // Druck-Kalibrierung 2
int16_t  dig_P3;  // Druck-Kalibrierung 3
int16_t  dig_P4;  // Druck-Kalibrierung 4
int16_t  dig_P5;  // Druck-Kalibrierung 5
int16_t  dig_P6;  // Druck-Kalibrierung 6
int16_t  dig_P7;  // Druck-Kalibrierung 7
int16_t  dig_P8;  // Druck-Kalibrierung 8
int16_t  dig_P9;  // Druck-Kalibrierung 9

// Globale Variable für Temperaturkompensation
// Wird von der Temperaturkompensation berechnet und für die Druckkompensation benötigt
int32_t t_fine;

// Initialisiert den BME280 Sensor
void bme280_init(void) {
	// Nur Temperatur und Druck aktivieren (keine Feuchtigkeit)
	// I2C-Start mit Schreibadresse
	i2cStart((BME280_ADDR << 1) | I2C_WRITE);
	i2cWrite(REG_CTRL_MEAS);  // Register-Adresse senden
	i2cWrite(0x27);           // Konfiguration: osrs_t=1, osrs_p=1, mode=normal
	i2cStop();                // I2C-Stop

	// Konfigurationsregister setzen
	i2cStart((BME280_ADDR << 1) | I2C_WRITE);
	i2cWrite(REG_CONFIG);     // Register-Adresse senden
	i2cWrite(0xA0);           // Konfiguration: Standby 1000ms, Filter aus
	i2cStop();                // I2C-Stop
}

// Liest die Kalibrierungsdaten vom BME280 Sensor
// Diese Daten sind notwendig für die Temperatur- und Druckkompensation
void bme280_read_calibration(void) {
        uint8_t calib[26];  // Puffer für 26 Kalibrierungsbytes
        
        // I2C-Start mit Schreibadresse für Kalibrierungsregister
        i2cStart((BME280_ADDR << 1) | I2C_WRITE);
        i2cWrite(0x88);      // Startadresse der Kalibrierungsdaten
        i2cStop();

        // I2C-Start mit Leseadresse
        i2cStart((BME280_ADDR << 1) | I2C_READ);
        
        // 25 Bytes mit ACK lesen
        for (uint8_t i = 0; i < 25; i++) {
	        calib[i] = i2cReadAck();
        }
        // Letztes Byte mit NACK lesen
        calib[25] = i2cReadNak();
        i2cStop();

        // Kalibrierungsdaten aus Bytes extrahieren (laut Datenblatt)
        // Temperatur-Kalibrierung (16-bit Werte)
        dig_T1 = (uint16_t)((calib[1] << 8) | calib[0]);  // T1: Bytes 0-1
        dig_T2 = (int16_t)((calib[3] << 8) | calib[2]);   // T2: Bytes 2-3
        dig_T3 = (int16_t)((calib[5] << 8) | calib[4]);   // T3: Bytes 4-5
        
        // Druck-Kalibrierung (16-bit Werte)
        dig_P1 = (uint16_t)((calib[7] << 8) | calib[6]);   // P1: Bytes 6-7
        dig_P2 = (int16_t)((calib[9] << 8) | calib[8]);    // P2: Bytes 8-9
        dig_P3 = (int16_t)((calib[11] << 8) | calib[10]);  // P3: Bytes 10-11
        dig_P4 = (int16_t)((calib[13] << 8) | calib[12]);  // P4: Bytes 12-13
        dig_P5 = (int16_t)((calib[15] << 8) | calib[14]);  // P5: Bytes 14-15
        dig_P6 = (int16_t)((calib[17] << 8) | calib[16]);  // P6: Bytes 16-17
        dig_P7 = (int16_t)((calib[19] << 8) | calib[18]);  // P7: Bytes 18-19
        dig_P8 = (int16_t)((calib[21] << 8) | calib[20]);  // P8: Bytes 20-21
        dig_P9 = (int16_t)((calib[23] << 8) | calib[22]);  // P9: Bytes 22-23
}

// Liest die Rohdaten (ADC-Werte) vom BME280 Sensor
// Diese Werte müssen noch kompensiert werden
void bme280_read_raw(int32_t* temp_raw, int32_t* press_raw) {
	uint8_t data[6];  // Puffer für 6 Datenbytes (3 Temperatur + 3 Druck)

	// I2C-Start mit Schreibadresse für Datenregister
	i2cStart((BME280_ADDR << 1) | I2C_WRITE);
	i2cWrite(REG_DATA);  // Startadresse der Messdaten
	i2cStop();

	// I2C-Start mit Leseadresse
	i2cStart((BME280_ADDR << 1) | I2C_READ);
	
	// 5 Bytes mit ACK lesen
	for (uint8_t i = 0; i < 5; i++) {
		data[i] = i2cReadAck();
	}
	// Letztes Byte mit NACK lesen
	data[5] = i2cReadNak();
	i2cStop();

	// Rohdaten aus Bytes extrahieren (20-bit Werte)
	// Druck: Bytes 0-2 (20-bit, rechtsbündig)
	*press_raw = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
	// Temperatur: Bytes 3-5 (20-bit, rechtsbündig)
	*temp_raw  = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);
}

// Kompensiert die Temperatur-Rohdaten (laut BME280-Datenblatt)
// Gibt Temperatur in 0.01°C zurück (z.B. 2350 = 23.50°C)
int32_t bme280_compensate_temp(int32_t adc_T) {
	int32_t var1, var2;  // Zwischenvariablen für Berechnung
	
	// Berechnung laut Datenblatt (Temperaturkompensation)
	var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * dig_T2) >> 11;
	var2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
	
	// t_fine berechnen (wird für Druckkompensation benötigt)
	t_fine = var1 + var2;
	
	// Kompensierte Temperatur zurückgeben (in 0.01°C)
	return (t_fine * 5 + 128) >> 8;
}

// Kompensiert die Druck-Rohdaten (laut BME280-Datenblatt)
// Gibt Druck in Pascal zurück (auf Meereshöhe reduziert)
uint32_t bme280_compensate_press(int32_t adc_P) {
	int32_t var1, var2;  // Zwischenvariablen für Berechnung
	uint32_t p;          // Kompensierter Druck

	// Berechnung laut Datenblatt (Druckkompensation)
	// Erste Zwischenberechnung
	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
	var2 = var2 + ((var1 * (int32_t)dig_P5) << 1);
	var2 = (var2 >> 2) + ((int32_t)dig_P4 << 16);
	
	// Zweite Zwischenberechnung
	var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + (((int32_t)dig_P2 * var1) >> 1)) >> 18;
	var1 = ((32768 + var1) * ((int32_t)dig_P1)) >> 15;
	
	// Division durch Null vermeiden
	if (var1 == 0) return 0;

	// Druckberechnung
	p = (((uint32_t)(((int32_t)1048576 - adc_P) - (var2 >> 12))) * 3125);
	if (p < 0x80000000) p = (p << 1) / (uint32_t)var1;
	else p = (p / (uint32_t)var1) * 2;

	// Zusätzliche Kompensation
	var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t)(p >> 2)) * (int32_t)dig_P8) >> 13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
	
	// Korrekturfaktor 1.04518 anwenden (für Meereshöhe)
	// Rechne über 64 Bit um Überlauf zu vermeiden
	uint64_t scaled_pressure = (uint64_t)p * 104518UL;
	p = (uint32_t)(scaled_pressure / 100000UL);

	return p;  // Druck in Pascal (auf Meereshöhe reduziert)
}

// Hauptfunktion: Liest Temperatur und Druck vom BME280
// Gibt kompensierte Werte in den gewünschten Einheiten zurück
void bmp280_read_temperature_and_pressure(int16_t* temp, uint16_t* press) {
	int32_t temp_raw, press_raw;  // Rohdaten vom Sensor
	
	// Rohdaten vom Sensor lesen
	bme280_read_raw(&temp_raw, &press_raw);
	
	// Temperatur kompensieren (gibt 0.01°C zurück)
	int32_t comp_temp = bme280_compensate_temp(temp_raw);
	*temp  = (int16_t)(comp_temp  / 10);  // In 0.1°C umrechnen
	
	// Druck kompensieren (gibt Pascal zurück)
	uint32_t comp_press = bme280_compensate_press(press_raw);
	*press = (uint16_t)(comp_press / 10);  // In 0.1 hPa umrechnen
}
