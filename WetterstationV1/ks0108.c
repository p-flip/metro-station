/* ks0108.c
 * Low-Level-Treiber für KS0108 (128×64 Pixel)
 */

#include "ks0108.h"


/* Bus-Write: verteilt ein Byte auf PB0–PB1 und PD2–PD7 */
static void write_bus(uint8_t v) {
    PORTB = (PORTB & ~(_BV(DB0_PIN)|_BV(DB1_PIN)))
          | ((v & 0x03) << DB0_PIN);
    PORTD = (PORTD & ~(_BV(DB2_PIN)|_BV(DB3_PIN)|_BV(DB4_PIN)
                     | _BV(DB5_PIN)|_BV(DB6_PIN)|_BV(DB7_PIN)))
          | (v & 0xFC);
}

/* Erzeugt einen kurzen Enable-Impuls */
static void pulse_enable(void) {
	//SPI-Hardware abschalten, damit wir PB5 als GPIO nutzen können
	SPCR &= ~(1 << SPE);
	
    SET(PORTB, EN_PIN);	
    _delay_us(1);
    CLR(PORTB, EN_PIN);
    _delay_us(1);
	
	// SPI-Hardware wieder einschalten, damit künftige SPI-Transfers SCK treiben
	SPCR |= (1 << SPE);
}

/* Initialisierung aller Pins und Aktivierung des Displays */
void ks0108_init(void) {
    /* Datenleitungen (DB0–DB7) als Ausgänge */
    DDRB |= _BV(DB0_PIN) | _BV(DB1_PIN) | _BV(DC_PIN) | _BV(RW_PIN) | _BV(EN_PIN);
    DDRD |= _BV(DB2_PIN) | _BV(DB3_PIN) | _BV(DB4_PIN)
         | _BV(DB5_PIN) | _BV(DB6_PIN) | _BV(DB7_PIN);
    /* Steuerleitungen (CS1, CS2, RST) als Ausgänge */
    DDRC |= _BV(CS1_PIN) | _BV(CS2_PIN) | _BV(RST_PIN);

    /* Hardware-Reset */
    CLR(PORTC, RST_PIN);
    _delay_ms(5);
    SET(PORTC, RST_PIN);
    _delay_ms(5);

    /* RW immer Write, Chips zunächst deaktiviert */
    CLR(PORTB, RW_PIN);
    SET(PORTC, CS1_PIN);
    SET(PORTC, CS2_PIN);
    _delay_ms(5);

    /* Display einschalten (0x3F = Display ON) */
    ks0108_write_command(0, 0x3F);
    ks0108_write_command(1, 0x3F);
    _delay_ms(10);
}

/* Befehl schreiben (chip = 0 linke Hälfte, 1 rechte Hälfte) */
void ks0108_write_command(uint8_t chip, uint8_t cmd) {
	//Definieren als Output (immer low)	
	PORTB  |=  (1 << RW_PIN);
	CLR(PORTB, RW_PIN);
	
    if (chip == 0) { CLR(PORTC, CS1_PIN); SET(PORTC, CS2_PIN); }
    else           { SET(PORTC, CS1_PIN); CLR(PORTC, CS2_PIN); }
    CLR(PORTB, DC_PIN);
    write_bus(cmd);
    pulse_enable();
    SET(PORTC, CS1_PIN);
    SET(PORTC, CS2_PIN);
	//Definieren als Input für EEPROM
	PORTB &= ~(1 << RW_PIN);
}

/* Databyte schreiben */
void ks0108_write_data(uint8_t chip, uint8_t data) {
	
	//Definieren als Output (immer low)
	PORTB  |=  (1 << RW_PIN);
	CLR(PORTB, RW_PIN);
	
    if (chip == 0) { CLR(PORTC, CS1_PIN); SET(PORTC, CS2_PIN); }
    else           { SET(PORTC, CS1_PIN); CLR(PORTC, CS2_PIN); }
    SET(PORTB, DC_PIN);
    write_bus(data);
    pulse_enable();
    SET(PORTC, CS1_PIN);
    SET(PORTC, CS2_PIN);
	
	//Definieren als Input für EEPROM
	PORTB &= ~(1 << RW_PIN);
}

/* Setzt die aktive Seite (0–7) */
void ks0108_set_page(uint8_t chip, uint8_t page) {
    ks0108_write_command(chip, 0xB8 | (page & 0x07));
}

/* Setzt die Spaltenadresse (0–63) */
void ks0108_set_column(uint8_t chip, uint8_t column) {
    ks0108_write_command(chip, 0x40 | (column & 0x3F));
}

/* Schreibt einen 128-Byte-Puffer in beide Display-Hälften */
void ks0108_write_page(uint8_t page, const uint8_t *buffer) {
		
    /* Linke Hälfte */
    ks0108_set_page(0, page);
    ks0108_set_column(0, 0);
    for (uint8_t c = 0; c < 64; c++) {
        ks0108_write_data(0, buffer[c]);
    }
    /* Rechte Hälfte */
    ks0108_set_page(1, page);
    ks0108_set_column(1, 0);
    for (uint8_t c = 0; c < 64; c++) {
        ks0108_write_data(1, buffer[c + 64]);
    }
}
