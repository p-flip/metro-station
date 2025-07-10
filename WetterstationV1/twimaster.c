/*
 * twimaster.c
 *
 * Hardware-I2C-Treiber für ATmega8 (TWI-Interface)
 * Implementiert die I2C-Kommunikation über die Hardware-TWI-Schnittstelle
 * Alternative zur Software-I2C-Implementierung in i2cMaster.c
 * 
 * Original: Peter Fleury <pfleury@gmx.ch>
 * Modified: 25.05.2025 14:11:18
 * Author: morri
 */ 

#include <inttypes.h>
#include <compat/twi.h>
#include "i2cMaster.h"

// CPU-Frequenz-Definition (falls nicht im Makefile definiert)
// Diese Frequenz wird für die TWI-Taktberechnung benötigt
#ifndef F_CPU
#define F_CPU 3686400UL
#endif

// I2C-Taktfrequenz in Hz
// Standard-I2C-Frequenz für die meisten Sensoren
#define SCL_CLOCK  100000L

// TWI-Initialisierung
// Konfiguriert die Hardware-TWI-Schnittstelle für I2C-Kommunikation
void i2c_init(void) {
	// TWI-Takt initialisieren: 100 kHz, TWPS = 0 => Prescaler = 1
	// TWSR (TWI Status Register) auf 0 setzen = kein Prescaler
	
	TWSR = 0;  // Kein Prescaler (TWPS1:0 = 00)
	
	// TWBR (TWI Bit Rate Register) berechnen
	// Formel: TWBR = ((F_CPU / SCL_CLOCK) - 16) / 2
	// Bei 3.6864 MHz und 100 kHz: TWBR = ((3686400 / 100000) - 16) / 2 = 2
	// Muss > 10 sein für stabile Operation
	TWBR = ((F_CPU/SCL_CLOCK)-16)/2;
}

// I2C-Start-Bedingung erzeugen und Geräteadresse senden
// Sendet eine Start-Bedingung und adressiert ein I2C-Gerät
// 
// Parameter: address - Geräteadresse mit R/W-Bit
// Rückgabe: 0 = Gerät erreichbar, 1 = Gerät nicht erreichbar
unsigned char i2c_start(unsigned char address) {
	uint8_t twst;  // TWI-Status-Variable
	
	// START-Bedingung senden
	// TWINT = 1 (Interrupt-Flag setzen), TWSTA = 1 (START senden), TWEN = 1 (TWI aktivieren)
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	
	// Warten bis Übertragung abgeschlossen ist
	// TWINT wird automatisch auf 1 gesetzt wenn Operation abgeschlossen ist
	while(!(TWCR & (1<<TWINT)));
	
	// TWI-Status-Register prüfen (Prescaler-Bits maskieren)
	// Nur die oberen 5 Bits enthalten den Status
	twst = TW_STATUS & 0xF8;
	
	// Prüfen ob START oder REPEATED START erfolgreich war
	if ((twst != TW_START) && (twst != TW_REP_START)) {
		return 1;  // START-Bedingung fehlgeschlagen
	}
	
	// Geräteadresse senden
	TWDR = address;  // Adresse in TWI Data Register schreiben
	TWCR = (1<<TWINT) | (1<<TWEN);  // Übertragung starten
	
	// Warten bis Übertragung abgeschlossen und ACK/NACK empfangen wurde
	while(!(TWCR & (1<<TWINT)));
	
	// TWI-Status-Register erneut prüfen
	twst = TW_STATUS & 0xF8;
	
	// Prüfen ob ACK empfangen wurde (Master Transmitter oder Master Receiver)
	if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK)) {
		return 1;  // Kein ACK empfangen = Gerät nicht erreichbar
	}
	
	return 0;  // Erfolgreich
}

// I2C-Start mit Warten (ACK-Polling)
// Sendet eine Start-Bedingung und wartet bis das Gerät bereit ist
// Verwendet ACK-Polling um auf ein beschäftigtes Gerät zu warten
// 
// Parameter: address - Geräteadresse mit R/W-Bit
void i2c_start_wait(unsigned char address) {
	uint8_t twst;  // TWI-Status-Variable
	
	// Schleife bis Gerät bereit ist
	while (1) {
		// START-Bedingung senden
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
		
		// Warten bis Übertragung abgeschlossen ist
		while(!(TWCR & (1<<TWINT)));
		
		// TWI-Status-Register prüfen
		twst = TW_STATUS & 0xF8;
		if ((twst != TW_START) && (twst != TW_REP_START)) {
			continue;  // START fehlgeschlagen, erneut versuchen
		}
		
		// Geräteadresse senden
		TWDR = address;
		TWCR = (1<<TWINT) | (1<<TWEN);
		
		// Warten bis Übertragung abgeschlossen ist
		while(!(TWCR & (1<<TWINT)));
		
		// TWI-Status-Register prüfen
		twst = TW_STATUS & 0xF8;
		
		// Prüfen ob Gerät beschäftigt ist (NACK empfangen)
		if ((twst == TW_MT_SLA_NACK) || (twst == TW_MR_DATA_NACK)) {
			// Gerät beschäftigt, STOP-Bedingung senden um Schreiboperation zu beenden
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
			
			// Warten bis STOP-Bedingung ausgeführt und Bus freigegeben ist
			while(TWCR & (1<<TWSTO));
			
			continue;  // Erneut versuchen
		}
		
		break;  // Gerät bereit, Schleife verlassen
	}
}

// I2C-Repeated Start-Bedingung
// Sendet eine wiederholte Start-Bedingung (ohne vorherigen STOP)
// 
// Parameter: address - Geräteadresse mit R/W-Bit
// Rückgabe: 0 = Gerät erreichbar, 1 = Gerät nicht erreichbar
unsigned char i2c_rep_start(unsigned char address) {
	// Repeated Start ist identisch zu normalem Start
	return i2c_start(address);
}

// I2C-Stop-Bedingung erzeugen
// Beendet die Datenübertragung und gibt den I2C-Bus frei
void i2c_stop(void) {
	// STOP-Bedingung senden
	// TWINT = 1 (Interrupt-Flag setzen), TWEN = 1 (TWI aktivieren), TWSTO = 1 (STOP senden)
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	
	// Warten bis STOP-Bedingung ausgeführt und Bus freigegeben ist
	// TWSTO wird automatisch auf 0 gesetzt wenn STOP abgeschlossen ist
	while(TWCR & (1<<TWSTO));
}

// Ein Byte an I2C-Gerät senden
// Sendet ein Datenbyte an das zuvor adressierte I2C-Gerät
// 
// Parameter: data - Zu sendendes Byte
// Rückgabe: 0 = Schreiben erfolgreich, 1 = Schreiben fehlgeschlagen
unsigned char i2c_write(unsigned char data) {
	uint8_t twst;  // TWI-Status-Variable
	
	// Daten an das zuvor adressierte Gerät senden
	TWDR = data;  // Daten in TWI Data Register schreiben
	TWCR = (1<<TWINT) | (1<<TWEN);  // Übertragung starten
	
	// Warten bis Übertragung abgeschlossen ist
	while(!(TWCR & (1<<TWINT)));
	
	// TWI-Status-Register prüfen (Prescaler-Bits maskieren)
	twst = TW_STATUS & 0xF8;
	
	// Prüfen ob ACK empfangen wurde
	if (twst != TW_MT_DATA_ACK) {
		return 1;  // Kein ACK = Schreiben fehlgeschlagen
	}
	
	return 0;  // Erfolgreich
}

// Ein Byte von I2C-Gerät lesen (mit ACK)
// Liest ein Byte vom I2C-Gerät und sendet ACK für weitere Daten
// 
// Rückgabe: Gelesenes Byte vom I2C-Gerät
unsigned char i2c_readAck(void) {
	// Byte lesen und ACK senden (für weitere Daten)
	// TWINT = 1 (Interrupt-Flag setzen), TWEN = 1 (TWI aktivieren), TWEA = 1 (ACK senden)
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	
	// Warten bis Übertragung abgeschlossen ist
	while(!(TWCR & (1<<TWINT)));
	
	// Gelesenes Byte aus TWI Data Register zurückgeben
	return TWDR;
}

// Ein Byte von I2C-Gerät lesen (mit NACK)
// Liest ein Byte vom I2C-Gerät und sendet NACK (letztes Byte)
// 
// Rückgabe: Gelesenes Byte vom I2C-Gerät
unsigned char i2c_readNak(void) {
	// Byte lesen und NACK senden (letztes Byte)
	// TWINT = 1 (Interrupt-Flag setzen), TWEN = 1 (TWI aktivieren), TWEA = 0 (NACK senden)
	TWCR = (1<<TWINT) | (1<<TWEN);
	
	// Warten bis Übertragung abgeschlossen ist
	while(!(TWCR & (1<<TWINT)));
	
	// Gelesenes Byte aus TWI Data Register zurückgeben
	return TWDR;
}
