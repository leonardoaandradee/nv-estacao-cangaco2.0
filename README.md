
## Integrantes do Grupo

- Eduardo Cadiz
- Larissa Beatriz
- Leonardo Andrade
- Kaio Guilherme

# Estação Meteorológica ESP32

Este projeto é uma estação meteorológica baseada no **ESP32** com sensor **DHT11**, sensor de chuva **Raindrops Module** e sensor de insolação (placa solar), exibindo dados em um display **SSD1306** e enviando informações para um broker **MQTT**.

## Funcionalidades

- Medição de **temperatura** e **umidade** com DHT11.
- Detecção de **chuva** via sensor analógico.
- Medição de **insolação** (luminosidade) via placa solar.
- LEDs indicativos: 
  - **Vermelho** acende quando a temperatura está acima do limite configurado (padrão: 25°C).
  - **Verde** indica temperatura normal.
  - **Vermelho** e **Verde** piscando simultaneamente indicam presença de chuva.
- **Buzzer** dispara sinal sonoro quando a temperatura ultrapassa o limite.
- **Tela OLED** exibindo:
  - Cabeçalho centralizado.
  - Dados de temperatura, umidade, chuva e insolação.
  - Tela de alerta com a palavra "ALERTA" em destaque centralizado, temperatura e status de chuva/insolação.
- Publicação dos dados nos tópicos MQTT:
  - `est_01/temp` → temperatura
  - `est_01/umid` → umidade
  - `est_01/chuva` → status de chuva ("Chovendo" ou "Sem chuva")
  - `est_01/solar` → insolação (0-100%)
  - `est_01/alerta` → "on" quando temperatura acima do limite, "off" caso contrário
- LEDs piscam simultâneos quando chuva detectada.

## Hardware Utilizado

- ESP32
- Sensor DHT11 (GPIO 32)
- Sensor de chuva analógico (GPIO 34)
- Placa solar (sensor de insolação, GPIO 33)
- LEDs verde (GPIO 15) e vermelho (GPIO 16)
- Buzzer (GPIO 2)
- Display SSD1306 I2C (SDA 21, SCL 22)

## Configurações

- **WiFi**: SSID e senha definidos no código-fonte
- **Broker MQTT**: `broker.hivemq.com`, porta `1883`, client ID `cangaco_01`
- **Tópicos MQTT**:
  - Temperatura: `est_01/temp`
  - Umidade: `est_01/umid`
  - Chuva: `est_01/chuva`
  - Insolação: `est_01/solar`
  - Alerta: `est_01/alerta` ("on" ou "off")

## Uso

1. Conecte o ESP32 aos sensores e display conforme os pinos configurados no código.
2. Faça upload do código para o ESP32 usando o Arduino IDE.
3. Abra o Serial Monitor para acompanhar a inicialização, leituras e status MQTT.
4. Monitore os tópicos MQTT para receber dados da estação em tempo real.

---

Projeto desenvolvido para a Faculdade Nova Roma / Recife, destinado à demonstração de sensores, MQTT e visualização em display OLED para a turma de Análise de Sistemas.
