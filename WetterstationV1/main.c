// --- DEBUG-MODUS ---
// Diese Zeile steuert, ob Debug-Ausgaben aktiviert sind
// 0 = Produktionsmodus (keine Debug-Ausgaben)
// 1 = Debug-Modus (mit UART-Ausgaben für Fehlersuche)
#define DEBUG_MODE 0 // Auf 0 für den finalen Code

// Standard AVR-Bibliotheken für Mikrocontroller-Funktionen
#include <avr/io.h>        // I/O-Register und Pin-Definitionen
#include <avr/interrupt.h> // Interrupt-Behandlung
#include <util/delay.h>    // Verzögerungsfunktionen
#include <stdlib.h>        // Standard-C-Funktionen (atoi, etc.)
#include <stdint.h>        // Standard-Integer-Typen
#include <stdbool.h>       // Boolean-Typ
#include <string.h>        // String-Funktionen

// Eigene Header-Dateien des Projekts
#include "Sensor.h"        // BME280 Sensor-Funktionen
#include "EEPROM.h"        // Externes EEPROM-Funktionen
#include "rs232.h"         // RS232-Kommunikation
#include "ks0108.h"        // KS0108 Display-Treiber
#include "display.h"       // Display-Rendering-Funktionen
#include "data.h"          // Globale Datenstrukturen
#include "i2cMaster.h"     // I2C-Kommunikation

// UART nur im Debug-Modus einbinden
#if DEBUG_MODE
#include "uart.h"          // UART für Debug-Ausgaben
#endif

// Struct-Definition für Sensordaten
// Speichert eine einzelne Messung mit Zeitstempel
typedef struct {
	uint32_t timestamp;  // Zeitstempel (in Sekunden seit Start)
	int16_t  temp;       // Temperatur in Zehntel-Grad (z.B. 235 = 23.5°C)
	uint16_t press;      // Druck in hPa (z.B. 1013 = 1013.0 hPa)
} SensorValue;

// Globale Variablen - werden in verschiedenen Funktionen verwendet
volatile uint8_t pageNumber = 1;    // Aktuelle Anzeigeseite (1-5)
volatile uint32_t timestamp = 0;    // Zeitstempel (wird von Timer-ISR erhöht)
char    cmd_buffer[4];              // Puffer für empfangene Befehle vom ESP8266
uint8_t cmd_index = 0;              // Aktuelle Position im Befehls-Puffer
int16_t  dataGraph[DISPLAY_COUNT];  // Daten für Display-Graphen (96 Werte)
int16_t  dataT;                     // Aktuelle Temperatur
uint16_t dataP;                     // Aktueller Druck
bool     first_run         = true;  // Flag für erste Ausführung
uint32_t last_measured     = 0;     // Zeitstempel der letzten Messung
uint32_t last_refrehed     = 0;     // Zeitstempel der letzten Display-Aktualisierung
uint8_t raw_index_24h = 0;          // Index für 24h-Rohdaten im EEPROM
uint8_t raw_index_7d  = 0;          // Index für 7-Tage-Rohdaten im EEPROM
uint8_t index_24h     = 0;          // Index für 24h-Durchschnittsdaten
uint8_t index_7d      = 0;          // Index für 7-Tage-Durchschnittsdaten

// Funktionsprototypen - Deklarationen für Funktionen, die später definiert werden
void timer1_init(void);
void store_sensor_value(uint16_t base_addr, uint8_t index, uint32_t ts, int16_t temp, uint16_t press);
void read_raw_values(uint16_t base_addr, uint8_t count, int32_t* temp_sum, uint32_t* press_sum, uint32_t* time_sum);
void loadDataGraph(uint8_t mode);
void send_data_packet(uint8_t page_num);

// Interrupt Service Routine für Timer1
// Wird alle ~1 Sekunde aufgerufen (abhängig von Timer-Konfiguration)
ISR(TIMER1_COMPA_vect) {
	timestamp++;  // Erhöht den Zeitstempel um 1
}

// Hauptprogramm - Startpunkt der Anwendung
int main(void) {
	// Initialisierung aller Hardware-Komponenten
	spi_init();        // SPI für externes EEPROM initialisieren
	initI2C();         // I2C für BME280 Sensor initialisieren
	timer1_init();     // Timer1 für Zeitmessung initialisieren
	rs232_init();      // RS232 für ESP8266-Kommunikation initialisieren

	// Debug-Modus: 10 Sekunden warten und UART initialisieren
	#if DEBUG_MODE
	_delay_ms(10000);  // 10 Sekunden warten für Debugging
	uart_init(7);      // UART mit Baudrate 7 initialisieren
	uart_send_string("DEBUG MODE ACTIVE\n");  // Debug-Nachricht senden
	#endif

	// Sensor und Display initialisieren
	bme280_init();                    // BME280 Sensor initialisieren
	bme280_read_calibration();        // Kalibrierungsdaten vom Sensor lesen
	ks0108_init();                    // KS0108 Display initialisieren
	
	// Taster-Pin konfigurieren (PC3 als Eingang mit Pull-up)
	DDRC  &= ~_BV(PC3);               // Pin als Eingang setzen
	PORTC |=  _BV(PC3);               // Pull-up-Widerstand aktivieren
	uint8_t last_button_state = 1;    // Letzter Taster-Zustand (1 = nicht gedrückt)
	
	sei();  // Interrupts global aktivieren

	// Hauptschleife - läuft endlos
	while (1) {
		// --- Taster-Abfrage für Seitenwechsel ---
		// Aktuellen Taster-Zustand lesen (0 = gedrückt, 1 = nicht gedrückt)
		uint8_t current_button_state = (PINC & _BV(PC3)) ? 1 : 0;
		
		// Taster wurde gerade gedrückt (fallende Flanke)
		if (last_button_state && !current_button_state) {
			_delay_ms(50);  // 50ms warten für Entprellung
			
			// Nochmal prüfen, ob Taster wirklich gedrückt ist
			if ((PINC & _BV(PC3)) == 0) {
				// Zur nächsten Seite wechseln (1->2->3->4->5->1...)
				pageNumber = (pageNumber % 5) + 1;

				// Display SOFORT aktualisieren
				loadDataGraph(pageNumber);  // Daten für neue Seite laden
				bmp280_read_temperature_and_pressure(&dataT, &dataP);  // Aktuelle Werte lesen
				
				// Alle Display-Seiten neu zeichnen
				for (uint8_t pg = 0; pg < SCREEN_H / 8; pg++) {
					clearPage(pg);                    // Seite löschen
					renderScene(pageNumber - 1, pg);  // Neue Szene rendern
					ks0108_write_page(pg, pageBuf);   // Seite zum Display senden
				}

				// Daten SOFORT an ESP8266 senden
				send_data_packet(pageNumber);

				// Timer zurücksetzen, um doppeltes Senden zu vermeiden
				last_refrehed = timestamp;
			}
		}
		last_button_state = current_button_state;  // Zustand für nächsten Durchlauf speichern

		// --- Periodische Aktualisierung und Daten Senden ---
		// Alle 6 Sekunden oder beim ersten Start
		if ((timestamp - last_refrehed) >= 6 || first_run) {
			last_refrehed = timestamp;  // Timer zurücksetzen

			// Display aktualisieren
			loadDataGraph(pageNumber);  // Daten laden
			bmp280_read_temperature_and_pressure(&dataT, &dataP);  // Aktuelle Werte lesen
			
			// Alle Display-Seiten neu zeichnen
			for (uint8_t pg = 0; pg < SCREEN_H / 8; pg++) {
				clearPage(pg);                    // Seite löschen
				renderScene(pageNumber - 1, pg);  // Szene rendern
				ks0108_write_page(pg, pageBuf);   // Seite senden
			}
			
			// Daten an ESP8266 senden
			send_data_packet(pageNumber);
		}

		// --- Sensor-Messung und EEPROM-Speicherung ---
		// Alle 2 Sekunden oder beim ersten Start
		if ((timestamp - last_measured) >= 2 || first_run) {
			last_measured = timestamp;  // Timer zurücksetzen
			
			// Aktuelle Sensordaten lesen
			bmp280_read_temperature_and_pressure(&dataT, &dataP);
			
			// Rohdaten für 24h-Speicherung
			store_sensor_value(EEPROM_ADDR_RAW_24H, raw_index_24h, timestamp, dataT, dataP);
			raw_index_24h = (raw_index_24h + 1) % RAW_BUFFER_24H_COUNT;  // Index erhöhen (Ringpuffer)
			
			// Rohdaten für 7-Tage-Speicherung
			store_sensor_value(EEPROM_ADDR_RAW_7D, raw_index_7d, timestamp, dataT, dataP);
			raw_index_7d = (raw_index_7d + 1) % RAW_BUFFER_7D_COUNT;  // Index erhöhen (Ringpuffer)
			
			// Wenn 24h-Ringpuffer voll ist, Durchschnitt berechnen
			if (raw_index_24h == 0) {
				int32_t temp_sum = 0; uint32_t press_sum = 0; uint32_t time_sum = 0;
				// Alle Rohdaten lesen und summieren
				read_raw_values(EEPROM_ADDR_RAW_24H, RAW_BUFFER_24H_COUNT, &temp_sum, &press_sum, &time_sum);
				// Durchschnitt speichern
				store_sensor_value(EEPROM_ADDR_24H, index_24h, time_sum / RAW_BUFFER_24H_COUNT, temp_sum / RAW_BUFFER_24H_COUNT, press_sum / RAW_BUFFER_24H_COUNT);
				index_24h = (index_24h + 1) % DISPLAY_COUNT;  // Index erhöhen (Ringpuffer)
			}
			
			// Wenn 7-Tage-Ringpuffer voll ist, Durchschnitt berechnen
			if (raw_index_7d == 0) {
				int32_t temp_sum = 0; uint32_t press_sum = 0; uint32_t time_sum = 0;
				// Alle Rohdaten lesen und summieren
				read_raw_values(EEPROM_ADDR_RAW_7D, RAW_BUFFER_7D_COUNT, &temp_sum, &press_sum, &time_sum);
				// Durchschnitt speichern
				store_sensor_value(EEPROM_ADDR_7D, index_7d, time_sum / RAW_BUFFER_7D_COUNT, temp_sum / RAW_BUFFER_7D_COUNT, press_sum / RAW_BUFFER_7D_COUNT);
				index_7d = (index_7d + 1) % DISPLAY_COUNT;  // Index erhöhen (Ringpuffer)
			}
			
			first_run = false;  // Erste Ausführung beendet
		}

		// --- Befehlsverarbeitung bei ANFRAGE vom ESP (für Seitenwechsel) ---
		// Prüfen, ob Daten vom ESP8266 verfügbar sind
		if (rs232_data_ready()) {
			char received = rs232_getchar();  // Ein Zeichen lesen
			
			// Wenn Zeilenende erreicht ist
			if (received == '\n' || received == '\r') {
				if (cmd_index > 0) {  // Wenn Befehls-Puffer nicht leer ist
					cmd_buffer[cmd_index] = '\0';  // String beenden
					uint8_t cmd = atoi(cmd_buffer);  // String zu Zahl konvertieren
					
					// Prüfen, ob gültige Seitennummer (1-5)
					if (cmd >= 1 && cmd <= 5) {
						pageNumber = cmd;  // Seite wechseln
						// Der nächste periodische Loop wird die Änderung übernehmen
					}
					cmd_index = 0;  // Puffer zurücksetzen
				}
			} else if (cmd_index < sizeof(cmd_buffer) - 1) {
				// Zeichen zum Puffer hinzufügen (mit Überlaufschutz)
				cmd_buffer[cmd_index++] = received;
			}
		}
	}
	return 0;  // Wird nie erreicht, da while(1) endlos läuft
}

// --- Implementierung der Hilfsfunktionen ---

// Sendet ein Datenpaket an den ESP8266
void send_data_packet(uint8_t page_num) {
	// Header im Format: d:X: senden
	rs232_putchar('d');           // Datenpaket-Kennung
	rs232_putchar(':');           // Trennzeichen
	rs232_putchar('0' + page_num); // Seitennummer als Zeichen
	rs232_putchar(':');           // Trennzeichen

	// Daten-Payload senden
	if (page_num <= 4) {
		// Für Seiten 1-4: Graph-Daten senden (96 Werte)
		for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
			rs232_send_int_semicolon(dataGraph[i]);  // Wert + Semikolon senden
		}
	} else {
		// Für Seite 5: Aktuelle Werte senden
		rs232_send_int_semicolon(dataT);  // Temperatur senden
		rs232_send_int_semicolon(dataP);  // Druck senden
	}
	// Paket mit Newline abschließen
	rs232_putchar('\n');
}

// Timer1 für Zeitmessung initialisieren
void timer1_init(void) {
	TCCR1B |= (1 << WGM12);           // CTC-Modus (Clear Timer on Compare Match)
	TCCR1B |= (1 << CS12) | (1 << CS10); // Prescaler 1024 (bei 3.6864 MHz = ~1 Sekunde)
	OCR1A   = 3599;                   // Compare-Wert für ~1 Sekunde Intervall
	TIMSK  |= (1 << OCIE1A);          // Timer1 Compare A Interrupt aktivieren
}

// Speichert einen Sensorwert im EEPROM
void store_sensor_value(uint16_t base_addr, uint8_t index, uint32_t ts, int16_t temp, uint16_t press) {
	SensorValue val;  // Temporäre Struktur für die Daten
	val.timestamp = ts;    // Zeitstempel setzen
	val.temp      = temp;  // Temperatur setzen
	val.press     = press; // Druck setzen
	
	// Adresse im EEPROM berechnen (Basis + Index * Größe der Struktur)
	uint16_t addr = base_addr + index * sizeof(SensorValue);
	
	// Struktur ins EEPROM schreiben
	eeprom_write_block(addr, (uint8_t*)&val, sizeof(SensorValue));
}

// Liest mehrere Rohwerte aus dem EEPROM und summiert sie
void read_raw_values(uint16_t base_addr, uint8_t count, int32_t* temp_sum, uint32_t* press_sum, uint32_t* time_sum) {
	SensorValue val;  // Temporäre Struktur für gelesene Daten
	
	// Summen initialisieren
	*temp_sum = 0; *press_sum = 0; *time_sum = 0;
	
	// Alle Werte lesen und summieren
	for (uint8_t i = 0; i < count; i++) {
		uint16_t addr = base_addr + i * sizeof(SensorValue);  // Adresse berechnen
		eeprom_read_block(addr, (uint8_t*)&val, sizeof(SensorValue));  // Daten lesen
		*temp_sum  += val.temp;      // Temperatur addieren
		*press_sum += val.press;     // Druck addieren
		*time_sum  += val.timestamp; // Zeitstempel addieren
	}
}

// Lädt Daten für Display-Graphen basierend auf der gewählten Seite
void loadDataGraph(uint8_t mode) {
	uint16_t base_addr;  // Basis-Adresse im EEPROM
	uint8_t  idx;        // Aktueller Index
	bool     wantTemp;   // Flag: Temperatur oder Druck?

	// Konfiguration basierend auf der Seite
	switch (mode) {
		case 1: base_addr = EEPROM_ADDR_24H; idx = index_24h; wantTemp = true;  break;  // Temp 24h
		case 2: base_addr = EEPROM_ADDR_24H; idx = index_24h; wantTemp = false; break;  // Druck 24h
		case 3: base_addr = EEPROM_ADDR_7D;  idx = index_7d;  wantTemp = true;  break;  // Temp 7 Tage
		case 4: base_addr = EEPROM_ADDR_7D;  idx = index_7d;  wantTemp = false; break;  // Druck 7 Tage
		default: return;  // Ungültige Seite
	}

	// 96 Datenpunkte für Display laden (rückwärts, da neueste Daten zuerst)
	for (uint8_t i = 0; i < DISPLAY_COUNT; i++) {
		uint8_t j = (idx + DISPLAY_COUNT - 1 - i) % DISPLAY_COUNT;  // Index rückwärts berechnen
		SensorValue sv;  // Temporäre Struktur
		uint16_t    addr = base_addr + j * sizeof(SensorValue);  // EEPROM-Adresse
		eeprom_read_block(addr, (uint8_t*)&sv, sizeof(sv));  // Daten lesen
		dataGraph[i] = wantTemp ? sv.temp : (int16_t)sv.press;  // Temperatur oder Druck speichern
	}
}