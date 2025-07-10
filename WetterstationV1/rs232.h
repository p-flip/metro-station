#ifndef RS232_H
#define RS232_H

#include <stdint.h>

void rs232_init(void);
void rs232_putchar(char c);
void rs232_puts(const char* s);
char rs232_getchar(void);
void rs232_send_int(int16_t value);
// NEUE FUNKTION: Sendet eine Ganzzahl gefolgt von einem Semikolon.
void rs232_send_int_semicolon(int16_t value);
uint8_t rs232_data_ready(void);

#endif