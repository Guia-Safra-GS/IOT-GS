// =====================================================================
// AgroMonitor — IoT ESP32
// Global Solution 2026/1 · FIAP · 2TDS
//
// Entradas:
//   - Potenciômetro  → GPIO34 (ADC) — simula umidade do solo
//   - DHT22          → GPIO4        — temperatura e umidade do ar
//   - Botão          → GPIO15       — passa página no OLED
//
// Saídas:
//   - OLED SSD1306   → I2C SDA=GPIO21 SCL=GPIO22
//   - LED RGB        → R=GPIO25 G=GPIO26 B=GPIO27
//
// WebServer:
//   - Porta 80 — página HTML estilizada com dados e status
//   - GET /        → página principal
//   - GET /status  → JSON com leituras atuais
// =====================================================================

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ------ Wi-Fi ------
const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

// ------ Pinos ------
#define PIN_SOIL_SENSOR  34
#define PIN_DHT          4
#define PIN_BUTTON       15
#define PIN_LED_R        25
#define PIN_LED_G        26
#define PIN_LED_B        27

// ------ OLED ------
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_RESET  -1
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ------ DHT22 ------
#define DHT_TYPE DHT22
DHT dht(PIN_DHT, DHT_TYPE);

// ------ WebServer ------
WebServer server(80);

// ------ Limiares de umidade do solo ------
const float THRESHOLD_LOW  = 40.0;  // abaixo → seco, precisa regar
const float THRESHOLD_HIGH = 70.0;  // acima  → solo ok

// ------ Variáveis de leitura ------
float soilHumidity   = 0.0;
float airTemperature = 0.0;
float airHumidity    = 0.0;

// ------ Status do solo ------
// 0 = OK (verde), 1 = ALERTA (amarelo), 2 = SECO (vermelho), 3 = ERRO (azul)
int soilStatus = 3;

// ------ Controle de tempo ------
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 2000; // leitura a cada 2s

// ------ Controle do botão e OLED ------
int   currentPage    = 0;
int   totalPages     = 3;
bool  lastButtonState = HIGH;

// =====================================================================
// LED RGB
// =====================================================================

void setLedOff() {
  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_G, LOW);
  digitalWrite(PIN_LED_B, LOW);
}

// Verde — solo ok
void setLedGreen() {
  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, LOW);
}

// Vermelho — solo seco
void setLedRed() {
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, LOW);
  digitalWrite(PIN_LED_B, LOW);
}

// Amarelo (R+G) — zona de alerta
void setLedYellow() {
  digitalWrite(PIN_LED_R, HIGH);
  digitalWrite(PIN_LED_G, HIGH);
  digitalWrite(PIN_LED_B, LOW);
}

// Azul — erro de leitura
void setLedBlue() {
  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_G, LOW);
  digitalWrite(PIN_LED_B, HIGH);
}

void updateLed() {
  switch (soilStatus) {
    case 0: setLedGreen();  break; // OK
    case 1: setLedYellow(); break; // Alerta
    case 2: setLedRed();    break; // Seco
    default: setLedBlue();  break; // Erro
  }
}

// =====================================================================
// Leitura dos sensores
// =====================================================================
void readSensors() {
  // Potenciômetro: 0-4095 → 0-100%
  int raw = analogRead(PIN_SOIL_SENSOR);
  soilHumidity = map(raw, 0, 4095, 0, 100);

  // DHT22
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    soilStatus = 3; // erro de leitura
    updateLed();
    Serial.println("[Erro] Falha na leitura do DHT22");
    return;
  }

  airTemperature = t;
  airHumidity    = h;

  // Define status do solo
  if (soilHumidity < THRESHOLD_LOW) {
    soilStatus = 2; // Seco
  } else if (soilHumidity < THRESHOLD_HIGH) {
    soilStatus = 1; // Alerta
  } else {
    soilStatus = 0; // OK
  }

  updateLed();

  Serial.printf("[Leitura] Solo: %.1f%% | Temp: %.1fC | Umid.Ar: %.1f%% | Status: %d\n",
    soilHumidity, airTemperature, airHumidity, soilStatus);
}

// =====================================================================
// OLED — Página 0: umidade do solo
// =====================================================================
void oledPage0() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("-- Umidade do Solo --"));

  display.setTextSize(2);
  display.setCursor(20, 20);
  display.print(soilHumidity, 1);
  display.println(F("%"));

  display.setTextSize(1);
  display.setCursor(0, 48);
  switch (soilStatus) {
    case 0: display.println(F("Status: SOLO OK")); break;
    case 1: display.println(F("Status: ATENCAO")); break;
    case 2: display.println(F("Status: SOLO SECO!")); break;
    default: display.println(F("Status: ERRO")); break;
  }

  display.setCursor(100, 56);
  display.print(F("1/3"));
  display.display();
}

// =====================================================================
// OLED — Página 1: temperatura e umidade do ar
// =====================================================================
void oledPage1() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("--- Temperatura ---"));

  display.setTextSize(2);
  display.setCursor(16, 14);
  display.print(airTemperature, 1);
  display.println(F(" C"));

  display.setTextSize(1);
  display.setCursor(0, 38);
  display.println(F("--- Umidade Ar ---"));

  display.setTextSize(1);
  display.setCursor(30, 52);
  display.print(airHumidity, 1);
  display.println(F(" %"));

  display.setCursor(100, 56);
  display.print(F("2/3"));
  display.display();
}

// =====================================================================
// OLED — Página 2: resumo geral
// =====================================================================
void oledPage2() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("--- AgroMonitor ---"));

  display.setCursor(0, 14);
  display.print(F("Solo:  "));
  display.print(soilHumidity, 1);
  display.println(F("%"));

  display.setCursor(0, 26);
  display.print(F("Temp:  "));
  display.print(airTemperature, 1);
  display.println(F(" C"));

  display.setCursor(0, 38);
  display.print(F("Ar:    "));
  display.print(airHumidity, 1);
  display.println(F("%"));

  // Aviso de rega
  display.setCursor(0, 52);
  if (soilStatus == 2) {
    display.println(F(">> PRECISA REGAR! <<"));
  } else if (soilStatus == 1) {
    display.println(F(">> ATENCAO AO SOLO <<"));
  } else {
    display.println(F(">> TUDO CERTO :)  <<"));
  }

  display.setCursor(100, 56);
  display.print(F("3/3"));
  display.display();
}

// =====================================================================
// Renderiza a página atual do OLED
// =====================================================================
void renderOledPage() {
  switch (currentPage) {
    case 0: oledPage0(); break;
    case 1: oledPage1(); break;
    case 2: oledPage2(); break;
  }
}

// =====================================================================
// Verifica botão — passa página no OLED
// =====================================================================
void checkButton() {
  bool currentState = digitalRead(PIN_BUTTON);

  // Detecta borda de descida (pressionado)
  if (lastButtonState == HIGH && currentState == LOW) {
    delay(50); // debounce simples
    currentPage = (currentPage + 1) % totalPages;
    renderOledPage();
    Serial.printf("[Botao] Pagina -> %d\n", currentPage + 1);
  }

  lastButtonState = currentState;
}

// =====================================================================
// HTML do WebServer
// =====================================================================
String buildHtmlPage() {
  String statusColor, statusBg, statusMsg, statusIcon, ledColor;

  switch (soilStatus) {
    case 0:
      statusColor = "#1B5E20"; statusBg = "#E8F5E9";
      statusMsg = "Solo com boa umidade — tudo certo!";
      statusIcon = "&#10003;"; ledColor = "#43A047";
      break;
    case 1:
      statusColor = "#E65100"; statusBg = "#FFF3E0";
      statusMsg = "Atenção — umidade no limite!";
      statusIcon = "&#9888;"; ledColor = "#FFA000";
      break;
    case 2:
      statusColor = "#B71C1C"; statusBg = "#FFEBEE";
      statusMsg = "Solo seco — necessita de rega!";
      statusIcon = "&#9888;"; ledColor = "#E53935";
      break;
    default:
      statusColor = "#1565C0"; statusBg = "#E3F2FD";
      statusMsg = "Aguardando leitura dos sensores...";
      statusIcon = "&#8987;"; ledColor = "#1E88E5";
      break;
  }

  int barWidth   = (int)constrain(soilHumidity, 0, 100);
  String barColor = (soilStatus == 2) ? "#E53935" : (soilStatus == 1) ? "#FFA000" : "#43A047";

  String html = "<!DOCTYPE html><html lang='pt-BR'><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<title>AgroMonitor</title>";
  html += "<style>";
  html += "*{box-sizing:border-box;margin:0;padding:0}";
  html += "body{font-family:'Segoe UI',sans-serif;background:#F1F8E9;color:#212121}";
  html += "header{background:#2E7D32;color:#fff;padding:20px 24px}";
  html += "header h1{font-size:22px;font-weight:600}";
  html += "header p{font-size:13px;opacity:.85;margin-top:4px}";
  html += ".container{max-width:500px;margin:24px auto;padding:0 16px}";
  html += ".status{border-radius:12px;padding:18px 20px;margin-bottom:18px;border:2px solid " + statusColor + ";background:" + statusBg + ";display:flex;align-items:center;gap:14px}";
  html += ".status-icon{font-size:30px}";
  html += ".status-text h2{font-size:15px;font-weight:600;color:" + statusColor + "}";
  html += ".status-text p{font-size:12px;color:#555;margin-top:4px}";
  html += ".led-row{display:flex;align-items:center;gap:10px;margin-bottom:18px;background:#fff;border-radius:10px;padding:14px 16px;box-shadow:0 1px 4px rgba(0,0,0,.08)}";
  html += ".led-circle{width:22px;height:22px;border-radius:50%;background:" + ledColor + ";flex-shrink:0}";
  html += ".led-label{font-size:13px;color:#424242}";
  html += ".cards{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px;margin-bottom:18px}";
  html += ".card{background:#fff;border-radius:10px;padding:16px 12px;box-shadow:0 1px 4px rgba(0,0,0,.08);text-align:center}";
  html += ".card .lbl{font-size:11px;color:#757575;text-transform:uppercase;letter-spacing:.5px;margin-bottom:6px}";
  html += ".card .val{font-size:24px;font-weight:600;color:#212121}";
  html += ".card .unit{font-size:12px;color:#9E9E9E}";
  html += ".bar-wrap{background:#fff;border-radius:10px;padding:16px;box-shadow:0 1px 4px rgba(0,0,0,.08);margin-bottom:18px}";
  html += ".bar-lbl{font-size:12px;color:#757575;text-transform:uppercase;letter-spacing:.5px;margin-bottom:8px}";
  html += ".bar-bg{background:#E0E0E0;border-radius:6px;height:14px;overflow:hidden}";
  html += ".bar-fill{height:100%;border-radius:6px;background:" + barColor + ";width:" + String(barWidth) + "%}";
  html += ".bar-info{display:flex;justify-content:space-between;margin-top:6px;font-size:12px;color:#616161}";
  html += ".footer{text-align:center;font-size:12px;color:#9E9E9E;margin-top:20px;padding-bottom:24px}";
  html += "</style></head><body>";

  html += "<header><h1>&#127807; AgroMonitor</h1><p>Monitoramento agrícola · atualiza a cada 5s</p></header>";
  html += "<div class='container'>";

  // Status
  html += "<div class='status'>";
  html += "<div class='status-icon'>" + statusIcon + "</div>";
  html += "<div class='status-text'><h2>" + statusMsg + "</h2>";
  html += "<p>Limiar mínimo: " + String(THRESHOLD_LOW, 0) + "% &nbsp;|&nbsp; Limiar ideal: " + String(THRESHOLD_HIGH, 0) + "%</p></div></div>";

  // LED indicador
  html += "<div class='led-row'><div class='led-circle'></div>";
  html += "<div class='led-label'>LED RGB: ";
  switch (soilStatus) {
    case 0: html += "Verde — solo ok"; break;
    case 1: html += "Amarelo — atenção"; break;
    case 2: html += "Vermelho — solo seco"; break;
    default: html += "Azul — aguardando"; break;
  }
  html += "</div></div>";

  // Cards
  html += "<div class='cards'>";
  html += "<div class='card'><div class='lbl'>Solo</div><div class='val'>" + String(soilHumidity, 1) + "<span class='unit'>%</span></div></div>";
  html += "<div class='card'><div class='lbl'>Temp.</div><div class='val'>" + String(airTemperature, 1) + "<span class='unit'>°C</span></div></div>";
  html += "<div class='card'><div class='lbl'>Umid. ar</div><div class='val'>" + String(airHumidity, 1) + "<span class='unit'>%</span></div></div>";
  html += "</div>";

  // Barra de umidade do solo
  html += "<div class='bar-wrap'><div class='bar-lbl'>Umidade do solo</div>";
  html += "<div class='bar-bg'><div class='bar-fill'></div></div>";
  html += "<div class='bar-info'><span>0%</span><span>" + String(soilHumidity, 1) + "%</span><span>100%</span></div></div>";

  html += "<div class='footer'>AgroMonitor · FIAP Global Solution 2026/1</div>";
  html += "</div></body></html>";

  return html;
}

// =====================================================================
// Handlers do WebServer
// =====================================================================
void handleRoot() {
  server.send(200, "text/html", buildHtmlPage());
}

void handleStatus() {
  String json = "{";
  json += "\"soil_humidity\":"   + String(soilHumidity, 2)    + ",";
  json += "\"air_temperature\":" + String(airTemperature, 2)  + ",";
  json += "\"air_humidity\":"    + String(airHumidity, 2)     + ",";
  json += "\"soil_status\":"     + String(soilStatus)         + ",";
  json += "\"threshold_low\":"   + String(THRESHOLD_LOW, 1)   + ",";
  json += "\"threshold_high\":"  + String(THRESHOLD_HIGH, 1);
  json += "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Rota nao encontrada");
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  Serial.begin(115200);

  // LED RGB
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  setLedBlue(); // azul no boot (aguardando)

  // Botão
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // DHT22
  dht.begin();

  // OLED
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[Erro] OLED nao encontrado!"));
    while (true);
  }

  // Tela de boot
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(18, 8);  display.println(F("AgroMonitor"));
  display.setCursor(8, 22);  display.println(F("FIAP GS 2026/1"));
  display.setCursor(0, 38);  display.println(F("Conectando WiFi..."));
  display.display();

  // Wi-Fi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Mostra IP no OLED
  display.clearDisplay();
  display.setCursor(0, 0);  display.println(F("WiFi conectado!"));
  display.setCursor(0, 14); display.println(F("Acesse no browser:"));
  display.setTextSize(1);
  display.setCursor(0, 30); display.println(WiFi.localIP().toString());
  display.setCursor(0, 48); display.println(F("Botao: passa pagina"));
  display.display();
  delay(3000);

  // Rotas
  server.on("/",       handleRoot);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("WebServer iniciado.");

  // Primeira leitura e renderização
  readSensors();
  renderOledPage();
}

// =====================================================================
// LOOP
// =====================================================================
void loop() {
  server.handleClient();

  unsigned long now = millis();

  // Leitura dos sensores a cada READ_INTERVAL
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;
    readSensors();
    renderOledPage(); // atualiza a página atual com novos dados
  }

  // Verifica botão para trocar página
  checkButton();
}
