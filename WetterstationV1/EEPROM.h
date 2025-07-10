/*
 * EEPROM.h
 *
 * Created: 25.05.2025 14:11:03
 *  Author: morri
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

#define RAW_BUFFER_24H_COUNT 3
#define RAW_BUFFER_7D_COUNT  21
#define DISPLAY_COUNT        96
#define DISPLAY_INTERVAL     300

#define EEPROM_ADDR_RAW_24H 0x0000
#define EEPROM_ADDR_RAW_7D  0x0100
#define EEPROM_ADDR_24H     0x0200
#define EEPROM_ADDR_7D      0x0500

void spi_init(void);
uint8_t spi_transfer(uint8_t data);
void eeprom_select(void);
void eeprom_deselect(void);
void eeprom_write_enable(void);
void eeprom_write_byte(uint16_t address, uint8_t data);
uint8_t eeprom_read_byte(uint16_t address);
uint8_t eeprom_read_status(void);
void eeprom_write_block(uint16_t address, const uint8_t* data, uint16_t length);
void eeprom_read_block(uint16_t address, uint8_t* data, uint16_t length);



#endif /* EEPROM_H_ */