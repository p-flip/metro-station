// ESP8266 Web-Interface für Wetterstation
// Erstellt einen WiFi Access Point und serviert eine HTML-Seite mit interaktiven Graphen
// Kommuniziert mit dem ATmega8 über RS232

#include <ESP8266WiFi.h>        // WiFi-Funktionen für ESP8266
#include <ESP8266WebServer.h>   // Web-Server für HTTP-Anfragen
#include <SoftwareSerial.h>     // Software-UART für RS232-Kommunikation

// RS232-Pin-Definitionen für Kommunikation mit ATmega8
#define RX_PIN 4  // GPIO4 = Empfangsleitung vom ATmega8
#define TX_PIN 5  // GPIO5 = Sendeleitung zum ATmega8
SoftwareSerial rs232(RX_PIN, TX_PIN);  // Software-UART-Objekt

// Web-Server auf Port 80 erstellen
ESP8266WebServer server(80);

// Globale Variablen zum Zwischenspeichern der Daten
volatile uint8_t page = 1;  // Aktuelle Seite (1-5)
String dataPayloads[6];     // Array für JSON-Daten jeder Seite (Index 0-5)

// Serielle Verarbeitung
String serialLine = "";  // Puffer für empfangene RS232-Zeilen

// Setup-Funktion - wird einmal beim Start ausgeführt
void setup() {
  // RS232 mit 28800 Baud initialisieren (muss mit ATmega8 übereinstimmen)
  rs232.begin(28800);
  delay(100);  // Kurz warten für Stabilisierung
  
  // Alle Daten-Puffer mit leeren JSON-Arrays initialisieren
  for (int i = 0; i < 6; i++) {
    dataPayloads[i] = "[]";
  }

  // WiFi Access Point konfigurieren
  WiFi.softAP("ESP_Sensor", "12345678");  // SSID und Passwort
  WiFi.setSleepMode(WIFI_NONE_SLEEP);     // WiFi-Sleep deaktivieren für bessere Performance

  // Route für die HTML-Seite (Hauptseite)
  server.on("/", HTTP_GET, []() {
    // HTML-Seite mit eingebettetem CSS und JavaScript senden
    server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <title>Sensor-Daten</title>
  <style>
    /* CSS-Variablen für Dark/Light Mode */
    :root {
      --bg-color: #fff;
      --text-color: #000;
      --panel-bg: #eee;
      --panel-active-bg: gray;
      --panel-active-text: #fff;
      --border-color: #666;
    }
    
    /* Dark Mode Farben */
    body.dark-mode {
      --bg-color: #121212;
      --text-color: #eee;
      --panel-bg: #333;
      --panel-active-bg: #555;
      --panel-active-text: #fff;
      --border-color: #aaa;
    }
    
    /* Grundlegende Styling */
    body {
      margin: 0; padding: 0;
      font-family: sans-serif;
      background-color: var(--bg-color);
      color: var(--text-color);
      transition: background-color 0.3s, color 0.3s;
    }
    
    h1 { text-align: center; margin-top: 1rem; }
    
    /* Steuerungselemente */
    #controls {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 1rem;
      margin: 1rem;
    }
    
    /* Radio-Buttons verstecken, Labels als Buttons verwenden */
    #page-selector input[type="radio"] { display: none; }
    #page-selector label {
      display: inline-block;
      margin: 0.2rem;
      padding: 0.5rem 1rem;
      background-color: var(--panel-bg);
      color: var(--text-color);
      border-radius: 4px;
      cursor: pointer;
      user-select: none;
      transition: background-color 0.3s, color 0.3s;
    }
    
    /* Aktiver Button */
    #page-selector label:has(input[type="radio"]:checked) {
      background-color: var(--panel-active-bg);
      color: var(--panel-active-text);
    }
    
    /* Canvas für Graphen */
    canvas {
      display: block;
      margin: 0 auto 1rem;
      border: 1px solid var(--border-color);
      transition: border-color 0.3s;
    }
    
    /* Aktuelle Werte Anzeige */
    #current {
      text-align: center;
      font-size: 1.2em;
      margin-top: 1rem;
    }
    
    /* Dark-Mode Umschalter */
    #dark-mode-toggle {
      cursor: pointer;
      width: 40px; height: 20px;
      appearance: none;
      background: var(--panel-bg);
      border-radius: 10px;
      position: relative;
      outline: none;
      transition: background 0.3s;
    }
    #dark-mode-toggle:checked {
      background: var(--panel-active-bg);
    }
    #dark-mode-toggle::after {
      content: "";
      position: absolute;
      top: 2px; left: 2px;
      width: 16px; height: 16px;
      background: var(--text-color);
      border-radius: 50%;
      transition: transform 0.3s;
    }
    #dark-mode-toggle:checked::after {
      transform: translateX(20px);
    }
  </style>
</head>
<body>
  <h1>Sensor-Daten</h1>
  
  <!-- Steuerungselemente -->
  <div id="controls">
    <div id="page-selector">
      <label><input type="radio" name="page" value="1" checked><span>Temp 24 h</span></label>
      <label><input type="radio" name="page" value="2"><span>Druck 24 h</span></label>
      <label><input type="radio" name="page" value="3"><span>Temp 7 Tage</span></label>
      <label><input type="radio" name="page" value="4"><span>Druck 7 Tage</span></label>
      <label><input type="radio" name="page" value="5"><span>Aktuelle Werte</span></label>
    </div>
    <label title="Dark Mode">
      <input type="checkbox" id="dark-mode-toggle">
      🌓
    </label>
  </div>

  <!-- Canvas für Graphen und Anzeige für aktuelle Werte -->
  <canvas id="chart" width="900" height="360"></canvas>
  <div id="current" style="display:none"></div>

  <script>
    // JavaScript-Variablen
    let pageJS = 1;  // Aktuelle Seite im JavaScript
    const canvas = document.getElementById('chart');
    const ctx    = canvas.getContext('2d');
    const curDiv = document.getElementById('current');
    const dmToggle = document.getElementById('dark-mode-toggle');

    // Dark-Mode-Umschaltung
    dmToggle.addEventListener('change', () => {
      document.body.classList.toggle('dark-mode', dmToggle.checked);
      // Chart neu zeichnen, um Kontraste anzupassen
      setPage(pageJS);
    });

    // Lädt Daten vom Server und aktualisiert die Anzeige
    function loadData(cmd) {
      pageJS = cmd;  // Aktuelle Seite speichern
      
      // Anzeige je nach Seite umschalten
      curDiv.style.display    = (cmd === 5 ? 'block' : 'none');  // Aktuelle Werte anzeigen
      canvas.style.display    = (cmd === 5 ? 'none' : 'block');  // Graph anzeigen

      // Daten vom Server abrufen (ohne Cache)
      fetch('/data?cmd=' + cmd, { cache: 'no-store' })
        .then(r => r.json())
        .then(arr => {
          if (cmd === 5) {
            // Seite 5: Aktuelle Werte als Text anzeigen
            curDiv.innerHTML =
              `<p>Temperatur: <strong>${(arr[0]/10).toFixed(1)} °C</strong></p>` +
              `<p>Druck:      <strong>${(arr[1]/10).toFixed(1)} hPa</strong></p>`;
          } else {
            // Seiten 1-4: Graph zeichnen
            drawChart(arr.map(v => v/10));  // Werte durch 10 teilen für Anzeige
          }
        })
        .catch(console.error);  // Fehler in Konsole ausgeben
    }

    // Setzt die aktuelle Seite und aktualisiert die Anzeige
    function setPage(n) {
      // Radio-Button entsprechend setzen
      document.querySelectorAll('#page-selector input[name="page"]').forEach(r => {
        r.checked = (parseInt(r.value) === n);
      });
      
      ctx.clearRect(0,0,canvas.width,canvas.height);  // Canvas löschen
      loadData(n);  // Daten laden
    }

    // Event-Listener für Radio-Buttons
    document.querySelectorAll('#page-selector input[name="page"]').forEach(radio => {
      radio.addEventListener('change', () => {
        const n = parseInt(radio.value);
        fetch('/page?num=' + n);  // Seite an ATmega8 senden
        setPage(n);  // Anzeige aktualisieren
      });
    });

    // Zeichnet einen Graphen mit den übergebenen Daten
    function drawChart(data) {
      // Canvas-Setup
      ctx.font = "14px sans-serif";
      const m = { top:20, right:20, bottom:60, left:80 };  // Margins
      const W = canvas.width, H = canvas.height;
      const cW = W - m.left - m.right, cH = H - m.top - m.bottom;  // Chart-Bereich
      const len = data.length - 1;
      ctx.clearRect(0,0,W,H);  // Canvas löschen

      // Gitternetz zeichnen
      const strokeGrid = getComputedStyle(document.body).getPropertyValue('--panel-bg').trim();
      ctx.strokeStyle = strokeGrid; ctx.setLineDash([5,5]);
      for (let i=0; i<=5; i++) {
        const y = m.top + cH * i / 5;
        ctx.beginPath(); ctx.moveTo(m.left, y); ctx.lineTo(m.left + cW, y); ctx.stroke();
      }
      ctx.setLineDash([]);

      // Min/Max-Werte für Y-Achse finden
      const min = Math.min(...data) - 0.0001;
      const max = Math.max(...data) + 0.0001;
      
      // Achsen zeichnen
      const axisColor = getComputedStyle(document.body).getPropertyValue('--text-color').trim();
      ctx.strokeStyle = axisColor;
      ctx.beginPath(); ctx.moveTo(m.left, m.top); ctx.lineTo(m.left, m.top + cH); ctx.lineTo(m.left + cW, m.top + cH); ctx.stroke();

      // Y-Achsen-Beschriftung
      ctx.fillStyle = axisColor; ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
      for (let i=0; i<=5; i++) {
        const yVal = max - (max - min) * (i / 5);
        const y = m.top + cH * (i / 5);
        ctx.beginPath(); ctx.moveTo(m.left - 5, y); ctx.lineTo(m.left, y); ctx.stroke();
        ctx.fillText(yVal.toFixed(1), m.left - 8, y);
      }

      // X-Achsen-Beschriftung
      ctx.textAlign = 'center'; ctx.textBaseline = 'top';
      const ticks = (pageJS <= 2 ? 25 : 15);  // Anzahl Ticks (24h vs 7d)
      for (let j=0; j<ticks; j++) {
        const x = m.left + cW * (j / (ticks - 1));
        ctx.beginPath(); ctx.moveTo(x, m.top + cH); ctx.lineTo(x, m.top + cH + 5); ctx.stroke();
        const lbl = (pageJS <= 2) ? `${j}` : `${(j*0.5).toFixed(1)}`;  // Stunden vs Tage
        ctx.fillText(lbl, x, m.top + cH + 8);
      }

      // Y-Achsen-Titel (gedreht)
      ctx.save(); ctx.translate(15, m.top + cH/2); ctx.rotate(-Math.PI/2);
      ctx.fillText((pageJS % 2 === 1 ? '°C' : 'hPa'), 0, 0);
      ctx.restore();

      // X-Achsen-Titel
      ctx.fillText((pageJS <= 2 ? 'Stunden' : 'Tage'), m.left + cW/2, m.top + cH + 40);

      // Graph-Linie zeichnen
      ctx.beginPath();
      data.forEach((v,i) => {
        const x = m.left + cW * (i / len);
        const y = m.top + cH * (1 - (v - min) / (max - min));
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      });
      ctx.strokeStyle = '#0077cc'; ctx.lineWidth = 2; ctx.stroke();
    }

    // Initiale Aufrufe
    setPage(1);  // Erste Seite laden
    
    // Automatische Updates alle 1 Sekunde
    setInterval(() => loadData(pageJS), 1000);
    
    // Synchronisation mit Hardware-Button alle 1 Sekunde
    setInterval(() => {
      fetch('/page').then(r => r.text()).then(text => {
        const n = parseInt(text);
        if (n >=1 && n <=5 && n !== pageJS) setPage(n);  // Seite wechseln, falls geändert
      }).catch(console.error);
    }, 1000);
  </script>
</body>
</html>
    )rawliteral");
  });

  // Route zum Setzen der Seite (von JavaScript aufgerufen)
  server.on("/page", HTTP_GET, []() {
    if (server.hasArg("num")) {
      // Neue Seite setzen
      int n = server.arg("num").toInt();
      if (n >= 1 && n <= 5) {
        page = n;  // Globale Variable setzen
        rs232.print(n);  // Seitennummer an ATmega8 senden
        rs232.print('\n');  // Zeilenende
      }
      server.send(200, "text/plain", "OK");
    } else {
      // Aktuelle Seite zurückgeben
      server.send(200, "text/plain", String(page));
    }
  });

  // Route für die JSON-Daten (von JavaScript aufgerufen)
  server.on("/data", HTTP_GET, []() {
    // No-Cache-Header senden, damit Browser immer neue Daten lädt
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    
    // Gewünschte Seite ermitteln
    int cmd = server.hasArg("cmd") ? server.arg("cmd").toInt() : page;
    if (cmd < 1 || cmd > 5) cmd = 1;  // Standard: Seite 1
    
    // JSON-Daten der gewünschten Seite senden
    server.send(200, "application/json", dataPayloads[cmd]);
  });

  // Web-Server starten
  server.begin();
}

// Verarbeitet empfangene RS232-Daten vom ATmega8
void processSerialData() {
  while (rs232.available()) {  // Solange Daten verfügbar sind
    char c = rs232.read();  // Ein Zeichen lesen
    
    if (c == '\n') {  // Zeilenende erreicht
      if (serialLine.startsWith("d:")) {  // Datenpaket erkannt
        // Format: d:X:data1;data2;data3;...
        int firstColon  = serialLine.indexOf(':');   // Erste Doppelpunkt-Position
        int secondColon = serialLine.indexOf(':', firstColon + 1);  // Zweite Doppelpunkt-Position
        
        if (secondColon > firstColon) {
          // Seitennummer extrahieren
          int receivedPage = serialLine.substring(firstColon + 1, secondColon).toInt();
          // Daten-String extrahieren
          String dataStr   = serialLine.substring(secondColon + 1);
          
          if (receivedPage >= 1 && receivedPage <= 5) {
            page = receivedPage;  // Globale Variable aktualisieren
            
            // JSON-Array aus Daten-String erstellen
            String json = "[";
            String num  = "";
            
            // Daten-String parsen (Semikolon-getrennt)
            for (unsigned int i = 0; i < dataStr.length(); i++) {
              if (dataStr[i] == ';') {
                // Semikolon gefunden - Zahl zum JSON hinzufügen
                if (num.length() > 0) { 
                  json += num + ","; 
                  num = ""; 
                }
              } else {
                // Zeichen zur aktuellen Zahl hinzufügen
                num += dataStr[i];
              }
            }
            
            // Letztes Komma entfernen, falls vorhanden
            if (json.endsWith(",")) json.remove(json.length() - 1);
            json += "]";
            
            // JSON in entsprechenden Puffer speichern
            dataPayloads[page] = json;
          }
        }
      }
      serialLine = "";  // Puffer zurücksetzen
    } else if (c != '\r') {  // Nicht-Carriage-Return Zeichen
      serialLine += c;  // Zeichen zum Puffer hinzufügen
    }
  }
}

// Hauptschleife - wird endlos ausgeführt
void loop() {
  processSerialData();  // RS232-Daten verarbeiten
  server.handleClient();  // Web-Server-Anfragen bearbeiten
}
