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
    módulo de detecção de precipitação ( Raindrop)
    Leds vermelho e verde
    Resistores 220 ohms
    Buzzer

*/
// Bibliotecas necessárias:
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Hardware:
#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define LED_VERDE 15
#define LED_VERMELHO 16
#define BUZZER 2
#define CHUVA_PIN 34

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi: Credenciais
const char* ssid = "ssid de seu wifi aqui";
const char* password = "coloque a senha de seu wifi aqui";

// Dados do MQTT:
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* clientID = "cangaco_01";
const char* topicChuva = "est_01/chuva";
const char* topicTemp = "est_01/temp";
const char* topicUmid = "est_01/umid";

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
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Conectando WiFi...");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("[INFO] WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi conectado!");
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
void drawNormalScreen(float temp, float umid, bool chovendo) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor((SCREEN_WIDTH - 7 * 15) / 2, 0);
  display.println("Estacao Cangaceiros");

  display.setCursor(0, 16);
  display.print("WiFi: ");
  display.println(WiFi.localIP());

  display.print("Temp: ");
  display.print(temp);
  display.println(" C");

  display.print("Umid: ");
  display.print(umid);
  display.println(" %");

  display.print("Chuva: ");
  display.println(chovendo ? "Sim" : "Nao");

  display.display();
}

// Exibição alerta:
void drawAlertScreen(float temp, bool chovendo) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor((SCREEN_WIDTH - 7 * 15) / 2, 0);
  display.println("Estacao Cangaceiros");

  display.setTextSize(2);
  display.setCursor(20, 24);
  display.print(temp);
  display.print(" C");

  display.setTextSize(1);
  display.setCursor(0, SCREEN_HEIGHT - 10); // rodapé
  display.print("Chuva: ");
  display.println(chovendo ? "Sim" : "Nao");

  display.display();
}

// Início de Setup:
void setup() {
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(BUZZER, LOW);

  Serial.begin(115200);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

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

    Serial.print("[SENSOR] Temp: ");
    Serial.print(temp);
    Serial.print(" C, Umid: ");
    Serial.print(umid);
    Serial.print("%, Chuva: ");
    Serial.println(chovendo ? "Sim" : "Nao");

    // Publicaões no MQTT:
    if (!isnan(temp) && !isnan(umid)) {
      client.publish(topicTemp, String(temp).c_str());
      client.publish(topicUmid, String(umid).c_str());
      Serial.println("[MQTT] Temperatura e Umidade publicadas");
    }
    client.publish(topicChuva, chovendo ? "Chovendo" : "Sem chuva");
    Serial.println("[MQTT] Chuva publicada");

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

    // Display (alerta):
    if (alertaTemperatura) {
      drawAlertScreen(temp, chovendo);
    } else {
      drawNormalScreen(temp, umid, chovendo);
    }
  }
}
