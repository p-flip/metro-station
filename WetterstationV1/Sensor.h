/*
 * Sensor.h
 *
 * Created: 25.05.2025 13:53:02
 *  Author: morri
 */ 


#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdint.h>



void bmp280_read_temperature_and_pressure(int16_t* temp, uint16_t* press);
void bme280_init(void);
void bme280_read_calibration(void);
void bme280_read_raw(int32_t* temp_raw, int32_t* press_raw);
int32_t bme280_compensate_temp(int32_t adc_T);
uint32_t bme280_compensate_press(int32_t adc_P);



#endif /* SENSOR_H_ */