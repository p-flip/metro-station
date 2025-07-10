# Wetterstation mit ATmega8 und ESP8266

Ein vollständiges Wetterstations-System bestehend aus einem ATmega8-Mikrocontroller für die Sensordaten-Erfassung und einem ESP8266 für das Web-Interface.

## 🏗️ Systemarchitektur

```
[BME280 Sensor] → [ATmega8] → [KS0108 Display]
                        ↓
                  [SPI EEPROM]
                        ↓
                  [RS232] → [ESP8266] → [WiFi] → [Web Browser]
```

## 📋 Komponenten

### Hardware
- **ATmega8** - Hauptmikrocontroller
- **BME280** - Temperatur- und Drucksensor (I2C)
- **KS0108** - 128x64 Grafik-LCD
- **SPI EEPROM** - Externer Speicher für historische Daten
- **ESP8266** - WiFi-Modul für Web-Interface
- **Taster** - Für manuellen Seitenwechsel

### Software
- **WetterstationV1/** - AVR-Code für ATmega8
- **webpageV7/** - Arduino-Code für ESP8266

## 🔧 Technische Spezifikationen

### ATmega8
- **CPU**: 3.6864 MHz
- **Flash**: 8KB
- **RAM**: 1KB
- **EEPROM**: 512 Bytes (intern) + externes SPI-EEPROM

### Kommunikation
- **I2C**: BME280 Sensor (100kHz)
- **SPI**: Externes EEPROM
- **RS232**: ESP8266 (28800 Baud)
- **WiFi**: ESP8266 Access Point

## 📊 Funktionen

### 5 Anzeigeseiten
1. **Temperatur 24h** - Graphische Darstellung der letzten 24 Stunden
2. **Druck 24h** - Luftdruck der letzten 24 Stunden
3. **Temperatur 7 Tage** - Wöchentliche Temperaturübersicht
4. **Druck 7 Tage** - Wöchentliche Druckübersicht
5. **Aktuelle Werte** - Live-Anzeige von Temperatur und Druck

### Datenspeicherung
- **Raw-Daten**: Alle 2 Sekunden Messungen
- **24h-Aggregation**: 3 Messungen → 1 Durchschnitt
- **7-Tage-Aggregation**: 21 Messungen → 1 Durchschnitt
- **Display-Buffer**: 96 Datenpunkte für Graphen

## 🚀 Installation

### Voraussetzungen
- AVR Studio / Atmel Studio (für ATmega8)
- Arduino IDE (für ESP8266)
- AVR-GCC Toolchain

### ATmega8 kompilieren
```bash
cd WetterstationV1
make clean
make
```

### ESP8266 flashen
1. Arduino IDE öffnen
2. `webpageV7.ino` laden
3. Board: "NodeMCU 1.0 (ESP-12E Module)" auswählen
4. Upload

## 🔌 Pin-Belegung

### ATmega8
```
BME280 (I2C):
- SDA: PC4
- SCL: PC5

KS0108 Display:
- DB0-DB7: PB0-PB1, PD2-PD7
- DC: PB2
- RW: PB4
- EN: PB5
- CS1: PC1
- CS2: PC0
- RST: PC2

SPI EEPROM:
- MOSI: PB3
- MISO: PB4
- SCK: PB5
- CS: PB2

RS232:
- TX: PD1
- RX: PD0

Taster:
- PC3 (mit Pull-up)
```

### ESP8266
```
RS232:
- RX: GPIO4
- TX: GPIO5
```

## 🌐 Web-Interface

### Zugriff
- **SSID**: ESP_Sensor
- **Passwort**: 12345678
- **IP**: 192.168.4.1
- **Port**: 80

### Features
- Responsive Design
- Dark/Light Mode
- Live-Updates (1s Intervall)
- Interaktive Graphen
- Synchronisation mit Hardware-Button

## 📡 Kommunikationsprotokoll

### ATmega8 → ESP8266
```
Format: d:X:data1;data2;data3;...\n
- X = Seitennummer (1-5)
- data = Semikolon-getrennte Werte
```

### ESP8266 → ATmega8
```
Format: X\n
- X = Seitennummer (1-5)
```

## 🔧 Konfiguration

### Debug-Modus
In `main.c`:
```c
#define DEBUG_MODE 0  // Auf 1 für Debug-Ausgabe
```

### Zeitintervalle
```c
#define DISPLAY_UPDATE_INTERVAL 6   // Sekunden
#define SENSOR_MEASURE_INTERVAL 2   // Sekunden
#define EEPROM_SAVE_INTERVAL    2   // Sekunden
```

## 🐛 Bekannte Probleme

1. **Race Condition**: Globale Variablen in ISR und main()
2. **EEPROM-Performance**: Ineffiziente Schreiboperationen
3. **Fehlende I2C-Fehlerbehandlung**: Keine Überprüfung von I2C-Transfers

## 🔮 Verbesserungsvorschläge

1. Atomare Operationen für globale Variablen
2. Optimierte EEPROM-Schreiboperationen
3. Bessere I2C-Fehlerbehandlung
4. Konfigurierbare Parameter
5. Erweiterte Debug-Unterstützung

## 📝 Lizenz

Dieses Projekt steht unter der MIT-Lizenz.

## 👨‍💻 Autor

Entwickelt für eine Wetterstations-Anwendung mit ATmega8 und ESP8266.

## 📞 Support

Bei Fragen oder Problemen bitte ein Issue erstellen.

---

**Hinweis**: Dieses Projekt wurde für den ATmega8 optimiert und nutzt dessen begrenzte Ressourcen effizient aus. 