/* ks0108.h
 * Low-Level-Treiber für KS0108 (128×64 Pixel)
 */
#ifndef KS0108_H
#define KS0108_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/*----- Pin-Definitionen gemäß Hardware -----*/
#define DB0_PIN   PB0
#define DB1_PIN   PB1
#define DB2_PIN   PD2
#define DB3_PIN   PD3
#define DB4_PIN   PD4
#define DB5_PIN   PD5
#define DB6_PIN   PD6
#define DB7_PIN   PD7

#define DC_PIN    PB2    /* Data/Command */
#define RW_PIN    PB4    /* RW = Write */
#define EN_PIN    PB5    /* Enable */

#define CS1_PIN   PC1    /* Chip Select linke Hälfte (active low) */
#define CS2_PIN   PC0    /* Chip Select rechte Hälfte (active low) */
#define RST_PIN   PC2    /* Hardware-Reset */

#define SET(p,b)  ((p) |=  (1<<(b)))
#define CLR(p,b)  ((p) &= ~(1<<(b)))

/*----- Funktionsprototypen -----*/
void ks0108_init(void);
void ks0108_write_command(uint8_t chip, uint8_t cmd);
void ks0108_write_data(uint8_t chip, uint8_t data);
void ks0108_set_page(uint8_t chip, uint8_t page);
void ks0108_set_column(uint8_t chip, uint8_t column);
void ks0108_write_page(uint8_t page, const uint8_t *buffer);

#endif /* KS0108_H */
