// --- DEBUG-MODUS ---
#define DEBUG_MODE 0 // Auf 0 für den finalen Code

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Eigene Header-Dateien
#include "Sensor.h"
#include "EEPROM.h"
#include "rs232.h"
#include "ks0108.h"
#include "display.h"
#include "data.h"
#include "i2cMaster.h"

#if DEBUG_MODE
#include "uart.h"
#endif

// Struct-Definition für Sensordaten
typedef struct {
	uint32_t timestamp;
	int16_t  temp;
	uint16_t press;
} SensorValue;

// Globale Variablen
volatile uint8_t pageNumber = 1;
volatile uint32_t timestamp = 0;
char    cmd_buffer[4];
uint8_t cmd_index = 0;
int16_t  dataGraph[DISPLAY_COUNT];
int16_t  dataT;
uint16_t dataP;
bool     first_run         = true;
uint32_t last_measured     = 0;
uint32_t last_refrehed     = 0;
uint8_t raw_index_24h = 0;
uint8_t raw_index_7d  = 0;
uint8_t index_24h     = 0;
uint8_t index_7d      = 0;

// Funktionsprototypen
void timer1_init(void);
void store_sensor_value(uint16_t base_addr, uint8_t index, uint32_t ts, int16_t temp, uint16_t press);
void read_raw_values(uint16_t base_addr, uint8_t count, int32_t* temp_sum, uint32_t* press_sum, uint32_t* time_sum);
void loadDataGraph(uint8_t mode);
void send_data_packet(uint8_t page_num);

// Interrupt Service Routine
ISR(TIMER1_COMPA_vect) {
	timestamp++;
}

// Hauptprogramm
int main(void) {
	// Initialisierung
	spi_init();
	initI2C();
	timer1_init();
	rs232_init();

	#if DEBUG_MODE
	_delay_ms(10000);
	uart_init(7);
	uart_send_string("DEBUG MODE ACTIVE\n");
	#endif

	bme280_init();
	bme280_read_calibration();
	ks0108_init();
	DDRC  &= ~_BV(PC3);
	PORTC |=  _BV(PC3);
	uint8_t last_button_state = 1;
	sei();

	while (1) {
		// --- Taster-Abfrage für Seitenwechsel ---
		uint8_t current_button_state = (PINC & _BV(PC3)) ? 1 : 0;
		if (last_button_state && !current_button_state) {
			_delay_ms(50);
			if ((PINC & _BV(PC3)) == 0) {
				pageNumber = (pageNumber % 5) + 1;

				// Lade Daten und zeichne das Display SOFORT neu
				loadDataGraph(pageNumber);
				bmp280_read_temperature_and_pressure(&dataT, &dataP);
				for (uint8_t pg = 0; pg < SCREEN_H / 8; pg++) {
					clearPage(pg);
					renderScene(pageNumber - 1, pg);
					ks0108_write_page(pg, pageBuf);
				}

				// Sende das Datenpaket SOFORT
				send_data_packet(pageNumber);

				// Timer zurücksetzen, um doppeltes Senden zu vermeiden
				last_refrehed = timestamp;
			}
		}
		last_button_state = current_button_state;

		// --- Periodische Aktualisierung und Daten Senden ---
		if ((timestamp - last_refrehed) >= 6 || first_run) {
			last_refrehed = timestamp;

			// Lade Daten und aktualisiere das Display
			loadDataGraph(pageNumber);
			bmp280_read_temperature_and_pressure(&dataT, &dataP);
			for (uint8_t pg = 0; pg < SCREEN_H / 8; pg++) {
				clearPage(pg);
				renderScene(pageNumber - 1, pg);
				ks0108_write_page(pg, pageBuf);
			}
			
			// Sende das Datenpaket im selben Intervall
			send_data_packet(pageNumber);
		}

		// --- Sensor-Messung und EEPROM-Speicherung ---
		if ((timestamp - last_measured) >= 2 || first_run) {
			last_measured = timestamp;
			bmp280_read_temperature_and_pressure(&dataT, &dataP);
			store_sensor_value(EEPROM_ADDR_RAW_24H, raw_index_24h, timestamp, dataT, dataP);
			raw_index_24h = (raw_index_24h + 1) % RAW_BUFFER_24H_COUNT;
			store_sensor_value(EEPROM_ADDR_RAW_7D, raw_index_7d, timestamp, dataT, dataP);
			raw_index_7d = (raw_index_7d + 1) % RAW_BUFFER_7D_COUNT;
			if (raw_index_24h == 0) {
				int32_t temp_sum = 0; uint32_t press_sum = 0; uint32_t time_sum = 0;
				read_raw_values(EEPROM_ADDR_RAW_24H, RAW_BUFFER_24H_COUNT, &temp_sum, &press_sum, &time_sum);
				store_sensor_value(EEPROM_ADDR_24H, index_24h, time_sum / RAW_BUFFER_24H_COUNT, temp_sum / RAW_BUFFER_24H_COUNT, press_sum / RAW_BUFFER_24H_COUNT);
				index_24h = (index_24h + 1) % DISPLAY_COUNT;
			}
			if (raw_index_7d == 0) {
				int32_t temp_sum = 0; uint32_t press_sum = 0; uint32_t time_sum = 0;
				read_raw_values(EEPROM_ADDR_RAW_7D, RAW_BUFFER_7D_COUNT, &temp_sum, &press_sum, &time_sum);
				store_sensor_value(EEPROM_ADDR_7D, index_7d, time_sum / RAW_BUFFER_7D_COUNT, temp_sum / RAW_BUFFER_7D_COUNT, press_sum / RAW_BUFFER_7D_COUNT);
				index_7d = (index_7d + 1) % DISPLAY_COUNT;
			}
			first_run = false;
		}

		// --- Befehlsverarbeitung bei ANFRAGE vom ESP (für Seitenwechsel) ---
		if (rs232_data_ready()) {
			char received = rs232_getchar();
			if (received == '\n' || received == '\r') {
				if (cmd_index > 0) {
					cmd_buffer[cmd_index] = '\0';
					uint8_t cmd = atoi(cmd_buffer);
					if (cmd >= 1 && cmd <= 5) {
						pageNumber = cmd;
						// Der nächste periodische Loop wird die Änderung übernehmen
					}
					cmd_index = 0;
				}
				} else if (cmd_index < sizeof(cmd_buffer) - 1) {
				cmd_buffer[cmd_index++] = received;
			}
		}
	}
	return 0;
}

// --- Implementierung der Hilfsfunktionen ---

void send_data_packet(uint8_t page_num) {
	// Sende Header im Format: d:X:
	rs232_putchar('d');
	rs232_putchar(':');
	rs232_putchar('0' + page_num);
	rs232_putchar(':');

	// Sende den Daten-Payload
	if (page_num <= 4) {
		for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
			rs232_send_int_semicolon(dataGraph[i]);
		}
		} else {
		rs232_send_int_semicolon(dataT);
		rs232_send_int_semicolon(dataP);
	}
	// Schließt das Paket mit einem Newline ab
	rs232_putchar('\n');
}

void timer1_init(void) {
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << CS12) | (1 << CS10);
	OCR1A   = 3599;
	TIMSK  |= (1 << OCIE1A);
}

void store_sensor_value(uint16_t base_addr, uint8_t index, uint32_t ts, int16_t temp, uint16_t press) {
	SensorValue val;
	val.timestamp = ts;
	val.temp      = temp;
	val.press     = press;
	uint16_t addr = base_addr + index * sizeof(SensorValue);
	eeprom_write_block(addr, (uint8_t*)&val, sizeof(SensorValue));
}

void read_raw_values(uint16_t base_addr, uint8_t count, int32_t* temp_sum, uint32_t* press_sum, uint32_t* time_sum) {
	SensorValue val;
	*temp_sum = 0; *press_sum = 0; *time_sum = 0;
	for (uint8_t i = 0; i < count; i++) {
		uint16_t addr = base_addr + i * sizeof(SensorValue);
		eeprom_read_block(addr, (uint8_t*)&val, sizeof(SensorValue));
		*temp_sum  += val.temp;
		*press_sum += val.press;
		*time_sum  += val.timestamp;
	}
}

void loadDataGraph(uint8_t mode) {
	uint16_t base_addr;
	uint8_t  idx;
	bool     wantTemp;

	switch (mode) {
		case 1: base_addr = EEPROM_ADDR_24H; idx = index_24h; wantTemp = true;  break;
		case 2: base_addr = EEPROM_ADDR_24H; idx = index_24h; wantTemp = false; break;
		case 3: base_addr = EEPROM_ADDR_7D;  idx = index_7d;  wantTemp = true;  break;
		case 4: base_addr = EEPROM_ADDR_7D;  idx = index_7d;  wantTemp = false; break;
		default: return;
	}

	for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
		uint8_t j = (idx + DISPLAY_COUNT - 1 - i) % DISPLAY_COUNT;
		SensorValue sv;
		uint16_t    addr = base_addr + j * sizeof(SensorValue);
		eeprom_read_block(addr, (uint8_t*)&sv, sizeof(sv));
		dataGraph[i] = wantTemp ? sv.temp : (int16_t)sv.press;
	}
}