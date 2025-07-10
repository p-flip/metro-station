/*
 * uart.h
 *
 * Created: 15.05.2025 11:33:13
 *  Author: morri
 */ 


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdio.h>

void uart_init(uint16_t ubrr);
void uart_send_char(char c);
void uart_send_string(const char* str);
void uart_send_uint8(uint8_t value);
void uart_send_hex8(uint8_t val);
void uart_send_hex32(uint32_t val);
void uart_send_int16(int16_t val);
void uart_send_int32(int32_t val);

#endif /* UART_H_ */ 