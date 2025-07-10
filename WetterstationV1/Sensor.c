/*
 * Sensor.c
 *
 * Created: 25.05.2025 13:53:11
 *  Author: morri
 */ 

#include "Sensor.h"
#include "i2cMaster.h"

#define BME280_ADDR 0x76
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG 0xF5
#define REG_DATA 0xF7

// Kalibrierungsvariablen (laut Datenblatt)
// Temperatur-Kalibrierwerte
uint16_t dig_T1;
int16_t  dig_T2;
int16_t  dig_T3;

// Druck-Kalibrierwerte
uint16_t dig_P1;
int16_t  dig_P2;
int16_t  dig_P3;
int16_t  dig_P4;
int16_t  dig_P5;
int16_t  dig_P6;
int16_t  dig_P7;
int16_t  dig_P8;
int16_t  dig_P9;

int32_t t_fine; // für Kompensation gebraucht


void bme280_init(void) {
	// Nur Temperatur und Druck aktivieren
	i2cStart((BME280_ADDR << 1) | I2C_WRITE);
	i2cWrite(REG_CTRL_MEAS);
	i2cWrite(0x27); // osrs_t=1, osrs_p=1, mode=normal
	i2cStop();

	i2cStart((BME280_ADDR << 1) | I2C_WRITE);
	i2cWrite(REG_CONFIG);
	i2cWrite(0xA0); // Standby 1000ms, Filter aus
	i2cStop();
}

void bme280_read_calibration(void) {
        uint8_t calib[26];
        i2cStart((BME280_ADDR << 1) | I2C_WRITE);
        i2cWrite(0x88);
        i2cStop();

        i2cStart((BME280_ADDR << 1) | I2C_READ);
        for (uint8_t i = 0; i < 25; i++) {
	        calib[i] = i2cReadAck();
        }
        calib[25] = i2cReadNak();
        i2cStop();

        dig_T1 = (uint16_t)((calib[1] << 8) | calib[0]);
        dig_T2 = (int16_t)((calib[3] << 8) | calib[2]);
        dig_T3 = (int16_t)((calib[5] << 8) | calib[4]);
        dig_P1 = (uint16_t)((calib[7] << 8) | calib[6]);
        dig_P2 = (int16_t)((calib[9] << 8) | calib[8]);
        dig_P3 = (int16_t)((calib[11] << 8) | calib[10]);
        dig_P4 = (int16_t)((calib[13] << 8) | calib[12]);
        dig_P5 = (int16_t)((calib[15] << 8) | calib[14]);
        dig_P6 = (int16_t)((calib[17] << 8) | calib[16]);
        dig_P7 = (int16_t)((calib[19] << 8) | calib[18]);
        dig_P8 = (int16_t)((calib[21] << 8) | calib[20]);
        dig_P9 = (int16_t)((calib[23] << 8) | calib[22]);
}

void bme280_read_raw(int32_t* temp_raw, int32_t* press_raw) {
	uint8_t data[6];

	i2cStart((BME280_ADDR << 1) | I2C_WRITE);
	i2cWrite(REG_DATA);
	i2cStop();

	i2cStart((BME280_ADDR << 1) | I2C_READ);
	for (uint8_t i = 0; i < 5; i++) {
		data[i] = i2cReadAck();
	}
	data[5] = i2cReadNak();
	i2cStop();

	*press_raw = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
	*temp_raw  = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);
}

int32_t bme280_compensate_temp(int32_t adc_T) {
	int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * dig_T2) >> 11;
	int32_t var2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
	t_fine = var1 + var2;
	return (t_fine * 5 + 128) >> 8; // °C * 100
}

uint32_t bme280_compensate_press(int32_t adc_P) {
	int32_t var1, var2;
	uint32_t p;

	var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
	var2 = var2 + ((var1 * (int32_t)dig_P5) << 1);
	var2 = (var2 >> 2) + ((int32_t)dig_P4 << 16);
	var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + (((int32_t)dig_P2 * var1) >> 1)) >> 18;
	var1 = ((32768 + var1) * ((int32_t)dig_P1)) >> 15;
	if (var1 == 0) return 0;

	p = (((uint32_t)(((int32_t)1048576 - adc_P) - (var2 >> 12))) * 3125);
	if (p < 0x80000000) p = (p << 1) / (uint32_t)var1;
	else p = (p / (uint32_t)var1) * 2;

	var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
	var2 = (((int32_t)(p >> 2)) * (int32_t)dig_P8) >> 13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
	// Korrekturfaktor 1.04518 ? 104518 / 100000 ? Rechne über 64 Bit
	uint64_t scaled_pressure = (uint64_t)p * 104518UL;
	p = (uint32_t)(scaled_pressure / 100000UL);

	return p;  // in Pascal (Pa), auf Meereshöhe reduziert
}

void bmp280_read_temperature_and_pressure(int16_t* temp, uint16_t* press) {
	int32_t temp_raw, press_raw;
	bme280_read_raw(&temp_raw, &press_raw);
	int32_t comp_temp = bme280_compensate_temp(temp_raw);
	*temp  = (int16_t)(comp_temp  / 10);
	uint32_t comp_press = bme280_compensate_press(press_raw);
	*press = (uint16_t)(comp_press / 10);
}
