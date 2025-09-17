/* 
  Faculdade Nova Roma - Recife / PE / BRasil
  Sistemas Embarcados - 02.2025

  Projeto: Estação Meteorológica Cangaceiros
  Equipe:
    Eduardo Cadiz
    Larissa Beatriz
    Kaio Guilherme
    Leonardo Andrade

  Hardware e compopnentes usados:
    Microcontrolador Esp32 
    Sensor de temperatura e umidade DHT11
    módulo de detecção de precipitação (Raindrop)
    Leds vermelho e verde
    Resistores: 2x 220 ohms | 1x 10k ohms
    Buzzer
    Placa solar 2V 160mA (sensor de insolação)
*/

// Bibliotecas:
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <DHT.h>

// Hardware:
#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define LED_VERDE 15
#define LED_VERMELHO 16
#define BUZZER 2
#define CHUVA_PIN 34
#define SOLAR_PIN 33  

// Display:
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// WiFi: Credenciais
const char* ssid = "**********";
const char* password = "*********";

// Dados do MQTT:
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* clientID = "cangaco_01";
const char* topicChuva = "est_01/chuva";
const char* topicTemp = "est_01/temp";
const char* topicUmid = "est_01/umid";
const char* topicSolar = "est_01/solar";
const char* topicAlerta = "est_01/alerta";

WiFiClient espClient;
PubSubClient client(espClient);

// Configuração:
const float TEMP_LIMITE = 25.00; // <- Ajuste aqui o limite de temperatura
unsigned long lastMsg = 0;
bool alertaTemperatura = false;

// Funções:
// Tela de boas-vindas
void showWelcomeScreen() {
  display.clearDisplay();
  u8g2Fonts.setFont(u8g2_font_helvR08_tr);
  
  // Mensagem boas vindas:
  const char* linha1 = "Estacao Meteorologica";
  const char* linha2 = "Cangaceiros";
  const char* linha3 = "est_01";
  
  int16_t x, y;
  uint16_t w, h;
  
  // Altura vertical do display:
  int alturaTotalTexto = 36; 
  int y_pos = (SCREEN_HEIGHT - alturaTotalTexto) / 2;
  
  // Centraliza e imprime linha 1
  display.getTextBounds(linha1, 0, 0, &x, &y, &w, &h);
  u8g2Fonts.setCursor((SCREEN_WIDTH - w) / 2, y_pos);
  u8g2Fonts.print(linha1);
  
  // Centraliza e imprime linha 2
  display.getTextBounds(linha2, 0, 0, &x, &y, &w, &h);
  u8g2Fonts.setCursor((SCREEN_WIDTH - w) / 2, y_pos + 14);
  u8g2Fonts.print(linha2);
  
  // Centraliza e imprime linha 3
  display.getTextBounds(linha3, 0, 0, &x, &y, &w, &h);
  u8g2Fonts.setCursor((SCREEN_WIDTH - w) / 2, y_pos + 28);
  u8g2Fonts.print(linha3);
  
  display.display();
  delay(5000); 
}

// Conexão WiFi:
bool connectWifi() {
  Serial.println("[INFO] Conectando ao WiFi...");
  WiFi.begin(ssid, password);

  display.clearDisplay();
  u8g2Fonts.setCursor(0, 12);
  u8g2Fonts.print("Conectando WiFi...");
  display.display();

  int tentativas = 0;
  int maxTentativas = 60;
  while (WiFi.status() != WL_CONNECTED && tentativas < maxTentativas) {
    Serial.print(".");
    int barraLargura = map(tentativas, 0, maxTentativas, 0, SCREEN_WIDTH-10);
    display.fillRect(5, 30, barraLargura, 10, SSD1306_WHITE);
    display.drawRect(5, 30, SCREEN_WIDTH-10, 10, SSD1306_WHITE);
    display.display();
    delay(250);
    tentativas++;
  }

  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[INFO] WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());
    
    display.clearDisplay();
    u8g2Fonts.setCursor(0, 12);
    u8g2Fonts.print("WiFi conectado!");
    display.display();
    delay(1000);
    return true;
  } else {
    Serial.println("[ERRO] Falha na conexão WiFi!");
    
    display.clearDisplay();
    u8g2Fonts.setCursor(0, 12);
    u8g2Fonts.print("WiFi: falha conexao");
    display.display();
    delay(1000);
    return false;
  }
}

// Reconecta MQTT:
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("[INFO] Conectando ao broker MQTT...");
    if (client.connect(clientID)) {
      Serial.println("[INFO] MQTT conectado!");
    } else {
      Serial.print("[ERRO] Falha MQTT, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// Exibição no display:
void drawNormalScreen(float temp, float umid, bool chovendo, float insolacao) {
  display.clearDisplay();
  const char* cabecalho = "Estacao Cangaceiros";
  u8g2Fonts.setFont(u8g2_font_helvR08_tr);
  int16_t x_cab, y_cab;
  uint16_t w_cab, h_cab;
  display.getTextBounds(cabecalho, 0, 0, &x_cab, &y_cab, &w_cab, &h_cab);
  int x_central = (SCREEN_WIDTH - w_cab) / 2;
  u8g2Fonts.setCursor((x_central+8), 12);
  u8g2Fonts.print(cabecalho);

  u8g2Fonts.setFont(u8g2_font_helvR08_tr);
  u8g2Fonts.setCursor(0, 28);
  u8g2Fonts.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    u8g2Fonts.print(WiFi.localIP());
  } else {
    u8g2Fonts.print("nao conectado");
  }

  u8g2Fonts.setCursor(0, 40);
  u8g2Fonts.print("Temp: ");
  u8g2Fonts.print(temp);
  u8g2Fonts.print(" C");

  u8g2Fonts.setCursor(0, 52);
  u8g2Fonts.print("Umid: ");
  u8g2Fonts.print(umid);
  u8g2Fonts.print(" %");

  u8g2Fonts.setCursor(0, 64);
  u8g2Fonts.print("Chuva: ");
  u8g2Fonts.print(chovendo ? "Sim" : "Nao");

  u8g2Fonts.setCursor(80, 52);
  u8g2Fonts.print("Luz: ");
  u8g2Fonts.print(insolacao, 0);
  u8g2Fonts.print("%");

  display.display();
}

// Exibição alerta:
void drawAlertScreen(float temp, bool chovendo, float insolacao) {
  display.clearDisplay();
  
  // Centralizar  verticalmente
  int alturaTotalTexto = 36;
  int y_pos_inicial = (SCREEN_HEIGHT - alturaTotalTexto) / 2;

  // Mensagem ALERTA centralizada horizontal e verticalmente
  u8g2Fonts.setFont(u8g2_font_fub14_tr);
  const char* alerta = "ALERTA";
  int16_t x_alert, y_alert;
  uint16_t w_alert, h_alert;
  display.getTextBounds(alerta, 0, 0, &x_alert, &y_alert, &w_alert, &h_alert);
  int x_alerta = (SCREEN_WIDTH - w_alert) / 2;

  u8g2Fonts.setCursor((x_alerta/2), y_pos_inicial);

  u8g2Fonts.print(alerta);

  // Exibe temperatura centralizada com mesmo tamanho de fonte:
  char temp_str[20];
  sprintf(temp_str, "%.1f oC", temp);
  display.getTextBounds(temp_str, 0, 0, &x_alert, &y_alert, &w_alert, &h_alert);
  int x_temp = (SCREEN_WIDTH - w_alert) / 2;
  u8g2Fonts.setCursor((x_temp/1.5), y_pos_inicial + 20);
  u8g2Fonts.print(temp_str);

  // Volta para fonte normal para informações adicionais
  u8g2Fonts.setFont(u8g2_font_helvR08_tr);
  
  // Informações na parte inferior da tela
  u8g2Fonts.setCursor(0, SCREEN_HEIGHT - 8);
  u8g2Fonts.print("Chuva: ");
  u8g2Fonts.print(chovendo ? "Sim" : "Nao");

  u8g2Fonts.setCursor(80, SCREEN_HEIGHT - 8);
  u8g2Fonts.print("Sol: ");
  u8g2Fonts.print(insolacao, 0);
  u8g2Fonts.print("%");

  display.display();
}

// Início de Setup:
void setup() {
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(SOLAR_PIN, INPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(BUZZER, LOW);

  Serial.begin(115200);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  u8g2Fonts.begin(display);  
  u8g2Fonts.setFont(u8g2_font_helvR08_tr);

  // Tela de boas-vindas:
  showWelcomeScreen();
  
  // Conecta WiFi com timeout de 15 segundos:
  bool wifiConnected = connectWifi();
  
  // Continua mesmo se o WiFi não conectar:
  client.setServer(mqtt_server, mqtt_port);
}

// Loop:
void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float temp = dht.readTemperature();
    float umid = dht.readHumidity();
    int chuvaVal = analogRead(CHUVA_PIN);
    bool chovendo = chuvaVal < 2000;

  // Leitura da placa solar:
  int leituraSolar = analogRead(SOLAR_PIN);
  const float offsetSolar = 0.4;      
  float tensaoSolar = (leituraSolar / 4095.0) * 3.3;
  float tensaoSolarCorrigida = tensaoSolar - offsetSolar;
  if (tensaoSolarCorrigida < 0) tensaoSolarCorrigida = 0;
  float insolacaoPercent = (tensaoSolarCorrigida / 2.0) * 100.0;
  if (insolacaoPercent > 100) insolacaoPercent = 100;

    Serial.print("[SENSOR] Temp: ");
    Serial.print(temp);
    Serial.print(" C, Umid: ");
    Serial.print(umid);
    Serial.print("%, Chuva: ");
    Serial.print(chovendo ? "Sim" : "Nao");
    Serial.print(", Solar: ");
    Serial.print(tensaoSolar);
    Serial.print(" V (");
    Serial.print(insolacaoPercent, 0);
    Serial.println("%)");

    // Publicações no MQTT:
    if (!isnan(temp) && !isnan(umid)) {
      client.publish(topicTemp, String(temp).c_str());
      client.publish(topicUmid, String(umid).c_str());
      Serial.println("[MQTT] Temperatura e Umidade publicadas");
    }
    client.publish(topicChuva, chovendo ? "Chovendo" : "Sem chuva");
    client.publish(topicSolar, String(insolacaoPercent, 0).c_str());
    Serial.println("[MQTT] Chuva e Insolacao publicadas");

    // Controle de temperatura:
    if (temp > TEMP_LIMITE) {
      digitalWrite(LED_VERMELHO, HIGH);
      digitalWrite(LED_VERDE, LOW);
      alertaTemperatura = true;
      Serial.println("[ALERTA] Temperatura acima do limite!");
      // Publica alerta no MQTT (apenas 'on'):
      client.publish(topicAlerta, "on");
      // Beep no buzzer: 
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);

    } else {
      digitalWrite(LED_VERMELHO, LOW);
      digitalWrite(LED_VERDE, HIGH);
      alertaTemperatura = false;
      digitalWrite(BUZZER, LOW);
      Serial.println("[INFO] Temperatura dentro do limite");
      // Publica alerta no MQTT (apenas 'off'):
      client.publish(topicAlerta, "off");
    }

    // Monitora chuva:
    if (chovendo) {
      Serial.println("[INFO] Chuva detectada! Piscar LEDs...");
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_VERMELHO, HIGH);
      delay(200);
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_VERMELHO, LOW);
      delay(200);
    }

    // Display:
    if (alertaTemperatura) {
      drawAlertScreen(temp, chovendo, insolacaoPercent);
    } else {
      drawNormalScreen(temp, umid, chovendo, insolacaoPercent);
    }
  }
}
