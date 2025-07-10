/*
 * uart.c
 *
 * UART-Treiber für ATmega8
 * Implementiert die serielle Kommunikation für Debugging und Datenübertragung
 * 
 * Created: 25.05.2025 14:11:18
 *  Author: morri
 */ 
//#define F_CPU 3686400UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "uart.h"

// UART-Puffer für eingehende Daten
// Ringpuffer-Implementierung für effiziente Datenverwaltung
volatile uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
volatile uint8_t uart_rx_head = 0;  // Schreibposition im Puffer
volatile uint8_t uart_rx_tail = 0;  // Leseposition im Puffer

// UART-Initialisierung für Debug-Kommunikation
// Konfiguriert Baudrate, Datenbits, Stoppbits und Interrupts
void uart_init(void) {
	// UBRR (USART Baud Rate Register) berechnen
	// Formel: UBRR = (F_CPU / (16 * BAUD)) - 1
	// Bei 3.6864 MHz und 9600 Baud: UBRR = (3686400 / (16 * 9600)) - 1 = 23
	UBRRH = 0;  // High-Byte des UBRR (0 bei 9600 Baud)
	UBRRL = 23; // Low-Byte des UBRR
	
	// UCSRC (USART Control and Status Register C) konfigurieren
	// URSEL = 1 (UCSRC wird geschrieben), UCSZ1:0 = 11 (8 Datenbits), USBS = 0 (1 Stoppbit)
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
	
	// UCSRB (USART Control and Status Register B) konfigurieren
	// RXEN = 1 (Receiver Enable), TXEN = 1 (Transmitter Enable), RXCIE = 1 (RX Complete Interrupt Enable)
	UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
}

// Ein Byte über UART senden (blockierend)
// Wartet bis der Transmitter bereit ist
void uart_putc(uint8_t data) {
	// Warten bis UDRE (USART Data Register Empty) gesetzt ist
	// Das bedeutet, der Transmitter ist bereit für neue Daten
	while (!(UCSRA & (1 << UDRE)));
	
	UDR = data;  // Daten in USART Data Register schreiben
}

// Ein String über UART senden (blockierend)
// Sendet Zeichen für Zeichen bis zum Null-Terminator
void uart_puts(const char* str) {
	while (*str) {  // Schleife bis Null-Terminator erreicht ist
		uart_putc(*str++);  // Aktuelles Zeichen senden und Pointer erhöhen
	}
}

// Prüfen ob Daten im Empfangspuffer verfügbar sind
// Gibt 1 zurück wenn Daten vorhanden, sonst 0
uint8_t uart_available(void) {
	// Wenn Head und Tail unterschiedlich sind, sind Daten im Puffer
	return (uart_rx_head != uart_rx_tail);
}

// Ein Byte aus dem Empfangspuffer lesen
// Gibt 0 zurück wenn keine Daten verfügbar sind
uint8_t uart_getc(void) {
	// Prüfen ob Daten verfügbar sind
	if (uart_rx_head == uart_rx_tail) {
		return 0;  // Keine Daten im Puffer
	}
	
	// Daten aus Puffer lesen
	uint8_t data = uart_rx_buffer[uart_rx_tail];
	
	// Tail-Pointer erhöhen (mit Überlauf-Schutz)
	uart_rx_tail = (uart_rx_tail + 1) % UART_RX_BUFFER_SIZE;
	
	return data;  // Gelesenes Byte zurückgeben
}

// UART Receive Complete Interrupt Service Routine
// Wird automatisch aufgerufen wenn ein Byte empfangen wurde
ISR(USART_RXC_vect) {
	// Empfangenes Byte aus UDR lesen
	uint8_t data = UDR;
	
	// Nächste Position im Ringpuffer berechnen
	uint8_t next_head = (uart_rx_head + 1) % UART_RX_BUFFER_SIZE;
	
	// Prüfen ob Puffer voll ist (Head würde Tail überholen)
	if (next_head != uart_rx_tail) {
		// Puffer nicht voll - Daten speichern
		uart_rx_buffer[uart_rx_head] = data;
		uart_rx_head = next_head;  // Head-Pointer erhöhen
	}
	// Wenn Puffer voll ist, werden die Daten verworfen (Overflow)
}

// Empfangspuffer leeren
// Setzt Head und Tail auf 0 zurück
void uart_flush_rx_buffer(void) {
	uart_rx_head = 0;  // Schreibposition zurücksetzen
	uart_rx_tail = 0;  // Leseposition zurücksetzen
}

// Mehrere Bytes über UART senden
// Sendet ein Array von Bytes mit angegebener Länge
void uart_send_bytes(const uint8_t* data, uint8_t length) {
	for (uint8_t i = 0; i < length; i++) {
		uart_putc(data[i]);  // Ein Byte senden
	}
}

// String mit Zeilenumbruch senden
// Fügt automatisch \r\n am Ende hinzu
void uart_puts_ln(const char* str) {
	uart_puts(str);     // String senden
	uart_putc('\r');    // Carriage Return
	uart_putc('\n');    // Line Feed
}

// Prüfen ob UART-Transmitter bereit ist
// Gibt 1 zurück wenn bereit, sonst 0
uint8_t uart_tx_ready(void) {
	return (UCSRA & (1 << UDRE));  // UDRE-Bit prüfen
}

// Prüfen ob UART-Empfänger Daten hat
// Gibt 1 zurück wenn Daten verfügbar, sonst 0
uint8_t uart_rx_ready(void) {
	return (UCSRA & (1 << RXC));   // RXC-Bit prüfen
}

// Integer-Wert als String über UART senden
// Konvertiert eine Zahl in einen String und sendet sie
void uart_send_int(int16_t value) {
	char buf[8];  // Puffer für String-Konvertierung
	itoa(value, buf, 10);  // Integer zu String konvertieren (Basis 10)
	uart_puts(buf);        // String senden
}

// Integer-Wert mit Semikolon senden
// Sendet eine Zahl gefolgt von einem Semikolon (für Daten-Streaming)
void uart_send_int_semicolon(int16_t value) {
	char buf[8];  // Puffer für String-Konvertierung
	itoa(value, buf, 10);  // Integer zu String konvertieren (Basis 10)
	uart_puts(buf);        // String senden
	uart_putc(';');        // Semikolon senden
}

// Integer-Wert mit Zeilenumbruch senden
// Sendet eine Zahl gefolgt von \r\n
void uart_send_int_ln(int16_t value) {
	uart_send_int(value);  // Zahl senden
	uart_putc('\r');       // Carriage Return
	uart_putc('\n');       // Line Feed
}

// Hexadezimalen Wert über UART senden
// Konvertiert ein Byte in Hex-String und sendet es
void uart_send_hex(uint8_t value) {
	char buf[3];  // Puffer für 2-stelligen Hex-String + Null-Terminator
	
	// High-Nibble konvertieren
	buf[0] = "0123456789ABCDEF"[value >> 4];
	
	// Low-Nibble konvertieren
	buf[1] = "0123456789ABCDEF"[value & 0x0F];
	
	buf[2] = '\0';  // Null-Terminator
	uart_puts(buf); // String senden
}

// Debug-Ausgabe mit Zeitstempel
// Sendet eine Debug-Nachricht mit aktueller Zeit
void uart_debug(const char* message) {
	uart_puts("[DEBUG] ");  // Debug-Präfix
	uart_puts(message);     // Nachricht senden
	uart_puts_ln("");       // Zeilenumbruch
}

// Fehler-Ausgabe
// Sendet eine Fehlermeldung mit Präfix
void uart_error(const char* message) {
	uart_puts("[ERROR] ");  // Error-Präfix
	uart_puts(message);     // Nachricht senden
	uart_puts_ln("");       // Zeilenumbruch
}

// Warnung-Ausgabe
// Sendet eine Warnung mit Präfix
void uart_warning(const char* message) {
	uart_puts("[WARN]  ");  // Warning-Präfix
	uart_puts(message);     // Nachricht senden
	uart_puts_ln("");       // Zeilenumbruch
}