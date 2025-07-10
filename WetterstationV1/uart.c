/*
 * uart.c
 *
 * Created: 15.05.2025 11:33:40
 *  Author: morri
 */ 

#include "uart.h"

void uart_init(uint16_t ubrr) {
	UBRRL = (uint8_t)ubrr;
	UBRRH = (uint8_t)(ubrr >> 8);
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // 8 Bit
	UCSRB = (1 << RXEN) | (1 << TXEN); // RX und TX aktivieren
}

void uart_send_char(char c) {
	while (!(UCSRA & (1 << UDRE)));
	UDR = c;
}

void uart_send_string(const char* str) {
	while (*str) {
		uart_send_char(*str++);
	}
}

void uart_send_uint8(uint8_t value) {
	char buffer[5]; // max. 3 Ziffern + '\0'
	sprintf(buffer, "%u", value);
	uart_send_string(buffer);
}
void uart_send_hex8(uint8_t val) {
	const char hex_chars[] = "0123456789ABCDEF";
	uart_send_char(hex_chars[(val >> 4) & 0x0F]);
	uart_send_char(hex_chars[val & 0x0F]);
}
void uart_send_hex32(uint32_t val) {
	for (int i = 28; i >= 0; i -= 4) {
		uint8_t nibble = (val >> i) & 0xF;
		if (nibble < 10) {
			uart_send_char(nibble + '0');
			} else {
			uart_send_char(nibble - 10 + 'A');
		}
	}
}
void uart_send_int16(int16_t val) {
	char buf[7]; // max "-32768\0"
	int i = 0;
	int isNegative = 0;

	if (val == 0) {
		uart_send_char('0');
		return;
	}
	if (val < 0) {
		isNegative = 1;
		val = -val;
	}

	// Ziffern umkehren sammeln
	while (val > 0) {
		buf[i++] = (val % 10) + '0';
		val /= 10;
	}
	if (isNegative) {
		buf[i++] = '-';
	}

	// Ausgabe in richtiger Reihenfolge
	for (int j = i - 1; j >= 0; j--) {
		uart_send_char(buf[j]);
	}
}

void uart_send_int32(int32_t val) {
	char buf[12]; // max "-2147483648\0"
	int i = 0;
	int isNegative = 0;

	if (val == 0) {
		uart_send_char('0');
		return;
	}
	if (val < 0) {
		isNegative = 1;
		val = -val;
	}

	// Ziffern rückwärts sammeln
	while (val > 0) {
		buf[i++] = (val % 10) + '0';
		val /= 10;
	}
	if (isNegative) {
		buf[i++] = '-';
	}

	// Ausgabe in korrekter Reihenfolge
	for (int j = i - 1; j >= 0; j--) {
		uart_send_char(buf[j]);
	}
}