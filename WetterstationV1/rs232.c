#include <avr/io.h>
#include <stdlib.h>
#include "rs232.h"

// Baudrate und Initialisierung bleiben unverändert
void rs232_init(void) {
	uint16_t ubrr = F_CPU / 16 / 28800 - 1;
	UBRRH = (uint8_t)(ubrr >> 8);
	UBRRL = (uint8_t)ubrr;
	UCSRA = 0;
	UCSRB = (1 << RXEN) | (1 << TXEN);
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

static void rs232_send_byte(uint8_t b) {
	while (!(UCSRA & (1 << UDRE)));
	UDR = b;
}

void rs232_putchar(char c) {
	rs232_send_byte((uint8_t)c);
}

void rs232_puts(const char* s) {
	while (*s) {
		rs232_send_byte((uint8_t)*s++);
	}
}

// NEUE FUNKTION: Sendet eine Zahl und ein Semikolon. Perfekt für Daten-Streaming.
void rs232_send_int_semicolon(int16_t value) {
	char buf[8];
	itoa(value, buf, 10);
	rs232_puts(buf);
	rs232_putchar(';');
}

// Alte Funktion (optional, kann beibehalten oder entfernt werden)
void rs232_send_int(int16_t value) {
	char buf[7];
	itoa(value, buf, 10);
	rs232_puts(buf);
	rs232_putchar(';');
	rs232_putchar('\n');
}

char rs232_getchar(void) {
	while (!(UCSRA & (1 << RXC)));
	return UDR;
}

uint8_t rs232_data_ready(void) {
	return (UCSRA & (1 << RXC)) != 0;
}