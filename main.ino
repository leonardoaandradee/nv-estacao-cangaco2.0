/* 
  Projeto Estação Meteorológica Cangaceiros
  Equipe:
    Eduardo Cadiz
    Larissa
    Kaio Guilherme
    Leonardo Andrade

  Hardware usado:
    Microcontrolador Esp32
    Sensor de temperatura e umidade DHT11
    módulo de detecção de precipitação (Raindrop)
    Leds vermelho e verde
    Resistores: 2x 220 ohms | 1x 10k ohms
    Buzzer
    Placa solar 2V 160mA (sensor de insolação)
*/

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
#define SOLAR_PIN 33   // Placa solar no ADC1 (GPIO33)

// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Adaptador de fontes

// WiFi: Credenciais
const char* ssid = "insira aqui sua wifi-ssid";
const char* password = "insira aqui a senha do seu wifi";

// Dados do MQTT:
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* clientID = "cangaco_01";
const char* topicChuva = "est_01/chuva";
const char* topicTemp = "est_01/temp";
const char* topicUmid = "est_01/umid";
const char* topicSolar = "est_01/solar";

WiFiClient espClient;
PubSubClient client(espClient);

// Configuração:
const float TEMP_LIMITE = 28.00;
unsigned long lastMsg = 0;
bool alertaTemperatura = false;

// Funções:
void connectWifi() {
  Serial.println("[INFO] Conectando ao WiFi...");
  WiFi.begin(ssid, password);

  display.clearDisplay();
  u8g2Fonts.setCursor(0, 12);
  u8g2Fonts.print("Conectando WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("[INFO] WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  u8g2Fonts.setCursor(0, 12);
  u8g2Fonts.print("WiFi conectado!");
  display.display();
  delay(1000);
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

  u8g2Fonts.setCursor(16, 12);
  u8g2Fonts.print("Estacao Cangaceiros");

  u8g2Fonts.setCursor(0, 28);
  u8g2Fonts.print("WiFi: ");
  u8g2Fonts.print(WiFi.localIP());

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
  u8g2Fonts.print("Sol: ");
  u8g2Fonts.print(insolacao, 0);
  u8g2Fonts.print("%");

  display.display();
}

// Exibição alerta:
void drawAlertScreen(float temp, bool chovendo, float insolacao) {
  display.clearDisplay();

  u8g2Fonts.setCursor(16, 12);
  u8g2Fonts.print("Estacao Cangaceiros");

  u8g2Fonts.setCursor(20, 36);
  u8g2Fonts.print("ALERTA!");
  
  u8g2Fonts.setCursor(0, 48);
  u8g2Fonts.print("Temp: ");
  u8g2Fonts.print(temp);
  u8g2Fonts.print(" C");

  u8g2Fonts.setCursor(0, 60);
  u8g2Fonts.print("Chuva: ");
  u8g2Fonts.print(chovendo ? "Sim" : "Nao");

  u8g2Fonts.setCursor(80, 60);
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

  connectWifi();
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

    // Leitura da placa solar
    int leituraSolar = analogRead(SOLAR_PIN);
    float tensaoSolar = (leituraSolar / 4095.0) * 3.3; 
    float insolacaoPercent = (tensaoSolar / 2.0) * 100.0;
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
