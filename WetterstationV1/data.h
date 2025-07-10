#ifndef DATA_H
#define DATA_H

#include <stdint.h>
#include <avr/pgmspace.h>

#define DISPLAY_COUNT 96

extern int16_t dataGraph[DISPLAY_COUNT];
/* Aktuelle Messwerte */
// globale, uninitialisierte Variablen für die aktuellen Werte:
extern int16_t  dataT;    // Temperatur in Zehntel-Grad
extern uint16_t dataP;    // Druck in hPa


#endif /* DATA_H */
