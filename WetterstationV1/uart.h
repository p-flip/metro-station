/*
 * uart.h
 *
 * Header-Datei für UART-Treiber
 * Definiert die Schnittstelle für serielle Kommunikation für Debugging
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

// UART-Konfiguration für Debug-Kommunikation
// Diese Werte bestimmen die serielle Kommunikation
#define UART_BAUDRATE     9600    // Baudrate für serielle Kommunikation
#define UART_RX_BUFFER_SIZE 64    // Größe des Empfangspuffers (Ringpuffer)

// UART-Initialisierung
// Konfiguriert die UART-Hardware für Debug-Kommunikation
void uart_init(void);

// Ein Byte über UART senden (blockierend)
// Wartet bis der Transmitter bereit ist
void uart_putc(uint8_t data);

// Ein String über UART senden (blockierend)
// Sendet Zeichen für Zeichen bis zum Null-Terminator
void uart_puts(const char* str);

// Prüfen ob Daten im Empfangspuffer verfügbar sind
// Gibt 1 zurück wenn Daten vorhanden, sonst 0
uint8_t uart_available(void);

// Ein Byte aus dem Empfangspuffer lesen
// Gibt 0 zurück wenn keine Daten verfügbar sind
uint8_t uart_getc(void);

// Empfangspuffer leeren
// Setzt Head und Tail auf 0 zurück
void uart_flush_rx_buffer(void);

// Mehrere Bytes über UART senden
// Sendet ein Array von Bytes mit angegebener Länge
void uart_send_bytes(const uint8_t* data, uint8_t length);

// String mit Zeilenumbruch senden
// Fügt automatisch \r\n am Ende hinzu
void uart_puts_ln(const char* str);

// Prüfen ob UART-Transmitter bereit ist
// Gibt 1 zurück wenn bereit, sonst 0
uint8_t uart_tx_ready(void);

// Prüfen ob UART-Empfänger Daten hat
// Gibt 1 zurück wenn Daten verfügbar, sonst 0
uint8_t uart_rx_ready(void);

// Integer-Wert als String über UART senden
// Konvertiert eine Zahl in einen String und sendet sie
void uart_send_int(int16_t value);

// Integer-Wert mit Semikolon senden
// Sendet eine Zahl gefolgt von einem Semikolon (für Daten-Streaming)
void uart_send_int_semicolon(int16_t value);

// Integer-Wert mit Zeilenumbruch senden
// Sendet eine Zahl gefolgt von \r\n
void uart_send_int_ln(int16_t value);

// Hexadezimalen Wert über UART senden
// Konvertiert ein Byte in Hex-String und sendet es
void uart_send_hex(uint8_t value);

// Debug-Ausgabe mit Zeitstempel
// Sendet eine Debug-Nachricht mit aktueller Zeit
void uart_debug(const char* message);

// Fehler-Ausgabe
// Sendet eine Fehlermeldung mit Präfix
void uart_error(const char* message);

// Warnung-Ausgabe
// Sendet eine Warnung mit Präfix
void uart_warning(const char* message);

// UART-Status-Informationen
// Struktur für UART-Statistiken und Status
typedef struct {
	uint16_t bytes_sent;       // Anzahl gesendeter Bytes
	uint16_t bytes_received;   // Anzahl empfangener Bytes
	uint16_t rx_overflows;     // Anzahl Empfangspuffer-Überläufe
	uint16_t tx_errors;        // Anzahl Sendefehler
	uint16_t rx_errors;        // Anzahl Empfangsfehler
	uint8_t  is_connected;     // Verbindungsstatus (1=verbunden, 0=getrennt)
} uart_status_t;

// UART-Status abrufen
// Liest aktuelle UART-Statistiken
void uart_get_status(uart_status_t* status);

// UART-Status zurücksetzen
// Setzt alle Zähler zurück
void uart_reset_status(void);

// UART-Verbindung testen
// Sendet ein Test-Signal und wartet auf Antwort
uint8_t uart_test_connection(void);

// UART-Echo-Modus aktivieren/deaktivieren
// Sendet empfangene Daten automatisch zurück
void uart_set_echo_mode(uint8_t enable);

// UART-Flow-Control konfigurieren
// Aktiviert/deaktiviert Hardware-Flow-Control (falls verfügbar)
void uart_set_flow_control(uint8_t enable);

// UART-Parameter konfigurieren
// Ändert Baudrate, Datenbits, Stoppbits, Parität
void uart_set_parameters(uint32_t baudrate, uint8_t data_bits, uint8_t stop_bits, uint8_t parity);

// UART-Interrupt-Handler
// Wird automatisch aufgerufen bei Empfang von Daten
// Diese Funktion ist in der .c-Datei als ISR implementiert
// void USART_RXC_vect(void);

#endif /* UART_H_ */ 