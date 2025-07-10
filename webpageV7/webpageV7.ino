#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

#define RX_PIN 4
#define TX_PIN 5
SoftwareSerial rs232(RX_PIN, TX_PIN);

ESP8266WebServer server(80);

// Globale Variablen zum Zwischenspeichern
volatile uint8_t page = 1;
String dataPayloads[6];

// Serielle Verarbeitung
String serialLine = "";

void setup() {
  rs232.begin(28800);
  delay(100);
  for (int i = 0; i < 6; i++) {
    dataPayloads[i] = "[]";
  }

  // WLAN-Access-Point
  WiFi.softAP("ESP_Sensor", "12345678");
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  // Route für die HTML-Seite
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <title>Sensor-Daten</title>
  <style>
    :root {
      --bg-color: #fff;
      --text-color: #000;
      --panel-bg: #eee;
      --panel-active-bg: gray;
      --panel-active-text: #fff;
      --border-color: #666;
    }
    body {
      margin: 0; padding: 0;
      font-family: sans-serif;
      background-color: var(--bg-color);
      color: var(--text-color);
      transition: background-color 0.3s, color 0.3s;
    }
    body.dark-mode {
      --bg-color: #121212;
      --text-color: #eee;
      --panel-bg: #333;
      --panel-active-bg: #555;
      --panel-active-text: #fff;
      --border-color: #aaa;
    }
    h1 { text-align: center; margin-top: 1rem; }
    #controls {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 1rem;
      margin: 1rem;
    }
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
    #page-selector label:has(input[type="radio"]:checked) {
      background-color: var(--panel-active-bg);
      color: var(--panel-active-text);
    }
    canvas {
      display: block;
      margin: 0 auto 1rem;
      border: 1px solid var(--border-color);
      transition: border-color 0.3s;
    }
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

  <canvas id="chart" width="900" height="360"></canvas>
  <div id="current" style="display:none"></div>

  <script>
    let pageJS = 1;
    const canvas = document.getElementById('chart');
    const ctx    = canvas.getContext('2d');
    const curDiv = document.getElementById('current');
    const dmToggle = document.getElementById('dark-mode-toggle');

    // Dark-Mode-Umschaltung
    dmToggle.addEventListener('change', () => {
      document.body.classList.toggle('dark-mode', dmToggle.checked);
      // ggf. Chart neu zeichnen, um Kontraste anzupassen
      setPage(pageJS);
    });

    function loadData(cmd) {
      pageJS = cmd;
      curDiv.style.display    = (cmd === 5 ? 'block' : 'none');
      canvas.style.display    = (cmd === 5 ? 'none' : 'block');

      fetch('/data?cmd=' + cmd, { cache: 'no-store' })
        .then(r => r.json())
        .then(arr => {
          if (cmd === 5) {
            curDiv.innerHTML =
              `<p>Temperatur: <strong>${(arr[0]/10).toFixed(1)} °C</strong></p>` +
              `<p>Druck:      <strong>${(arr[1]/10).toFixed(1)} hPa</strong></p>`;
          } else {
            drawChart(arr.map(v => v/10));
          }
        })
        .catch(console.error);
    }

    function setPage(n) {
      document.querySelectorAll('#page-selector input[name="page"]').forEach(r => {
        r.checked = (parseInt(r.value) === n);
      });
      ctx.clearRect(0,0,canvas.width,canvas.height);
      loadData(n);
    }

    document.querySelectorAll('#page-selector input[name="page"]').forEach(radio => {
      radio.addEventListener('change', () => {
        const n = parseInt(radio.value);
        fetch('/page?num=' + n);
        setPage(n);
      });
    });

    function drawChart(data) {
      // (unveränderte Zeichenroutine, nutzt CSS-Variablen für Farben)
      ctx.font = "14px sans-serif";
      const m = { top:20, right:20, bottom:60, left:80 };
      const W = canvas.width, H = canvas.height;
      const cW = W - m.left - m.right, cH = H - m.top - m.bottom;
      const len = data.length - 1;
      ctx.clearRect(0,0,W,H);

      const strokeGrid = getComputedStyle(document.body).getPropertyValue('--panel-bg').trim();
      ctx.strokeStyle = strokeGrid; ctx.setLineDash([5,5]);
      for (let i=0; i<=5; i++) {
        const y = m.top + cH * i / 5;
        ctx.beginPath(); ctx.moveTo(m.left, y); ctx.lineTo(m.left + cW, y); ctx.stroke();
      }
      ctx.setLineDash([]);

      const min = Math.min(...data) - 0.0001;
      const max = Math.max(...data) + 0.0001;
      const axisColor = getComputedStyle(document.body).getPropertyValue('--text-color').trim();
      ctx.strokeStyle = axisColor;
      ctx.beginPath(); ctx.moveTo(m.left, m.top); ctx.lineTo(m.left, m.top + cH); ctx.lineTo(m.left + cW, m.top + cH); ctx.stroke();

      ctx.fillStyle = axisColor; ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
      for (let i=0; i<=5; i++) {
        const yVal = max - (max - min) * (i / 5);
        const y = m.top + cH * (i / 5);
        ctx.beginPath(); ctx.moveTo(m.left - 5, y); ctx.lineTo(m.left, y); ctx.stroke();
        ctx.fillText(yVal.toFixed(1), m.left - 8, y);
      }

      ctx.textAlign = 'center'; ctx.textBaseline = 'top';
      const ticks = (pageJS <= 2 ? 25 : 15);
      for (let j=0; j<ticks; j++) {
        const x = m.left + cW * (j / (ticks - 1));
        ctx.beginPath(); ctx.moveTo(x, m.top + cH); ctx.lineTo(x, m.top + cH + 5); ctx.stroke();
        const lbl = (pageJS <= 2) ? `${j}` : `${(j*0.5).toFixed(1)}`;
        ctx.fillText(lbl, x, m.top + cH + 8);
      }

      ctx.save(); ctx.translate(15, m.top + cH/2); ctx.rotate(-Math.PI/2);
      ctx.fillText((pageJS % 2 === 1 ? '°C' : 'hPa'), 0, 0);
      ctx.restore();

      ctx.fillText((pageJS <= 2 ? 'Stunden' : 'Tage'), m.left + cW/2, m.top + cH + 40);

      ctx.beginPath();
      data.forEach((v,i) => {
        const x = m.left + cW * (i / len);
        const y = m.top + cH * (1 - (v - min) / (max - min));
        i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
      });
      ctx.strokeStyle = '#0077cc'; ctx.lineWidth = 2; ctx.stroke();
    }

    // Initiale Aufrufe
    setPage(1);
    setInterval(() => loadData(pageJS), 1000);
    setInterval(() => {
      fetch('/page').then(r => r.text()).then(text => {
        const n = parseInt(text);
        if (n >=1 && n <=5 && n !== pageJS) setPage(n);
      }).catch(console.error);
    }, 1000);
  </script>
</body>
</html>
    )rawliteral");
  });

  // Route zum Setzen der Seite
  server.on("/page", HTTP_GET, []() {
    if (server.hasArg("num")) {
      int n = server.arg("num").toInt();
      if (n >= 1 && n <= 5) {
        page = n;
        rs232.print(n);
        rs232.print('\n');
      }
      server.send(200, "text/plain", "OK");
    } else {
      server.send(200, "text/plain", String(page));
    }
  });

  // Route für die JSON-Daten mit No-Cache-Headern
  server.on("/data", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    int cmd = server.hasArg("cmd") ? server.arg("cmd").toInt() : page;
    if (cmd < 1 || cmd > 5) cmd = 1;
    server.send(200, "application/json", dataPayloads[cmd]);
  });

  server.begin();
}

void processSerialData() {
  while (rs232.available()) {
    char c = rs232.read();
    if (c == '\n') {
      if (serialLine.startsWith("d:")) {
        int firstColon  = serialLine.indexOf(':');
        int secondColon = serialLine.indexOf(':', firstColon + 1);
        if (secondColon > firstColon) {
          int receivedPage = serialLine.substring(firstColon + 1, secondColon).toInt();
          String dataStr   = serialLine.substring(secondColon + 1);
          if (receivedPage >= 1 && receivedPage <= 5) {
            page = receivedPage;
            String json = "[";
            String num  = "";
            for (unsigned int i = 0; i < dataStr.length(); i++) {
              if (dataStr[i] == ';') {
                if (num.length() > 0) { json += num + ","; num = ""; }
              } else {
                num += dataStr[i];
              }
            }
            if (json.endsWith(",")) json.remove(json.length() - 1);
            json += "]";
            dataPayloads[page] = json;
          }
        }
      }
      serialLine = "";
    } else if (c != '\r') {
      serialLine += c;
    }
  }
}

void loop() {
  processSerialData();
  server.handleClient();
}
