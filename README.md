# Estação Meteorológica ESP32

Este projeto é uma estação meteorológica simples baseada no **ESP32** com sensor **DHT11** e sensor de chuva **Raindrops Module**, exibindo dados em um display **SSD1306** e enviando informações para um broker **MQTT**.

## Funcionalidades

- Medição de **temperatura** e **umidade** com DHT11.
- Detecção de **chuva** via sensor analógico.
- LEDs indicativos: 
  - **Vermelho** acende quando a temperatura está acima de 28,5°C.
  - **Verde** indica temperatura normal.
- **Buzzer** dispara sinal sonoro quando a temperatura ultrapassa o limite.
- **Tela OLED** exibindo:
  - Cabeçalho centralizado.
  - Dados de temperatura, umidade e chuva.
  - Tela de alerta com temperatura em fonte maior e indicação de chuva no rodapé.
- Publicação dos dados nos tópicos MQTT:
  - `temp` → temperatura
  - `umid` → umidade
  - `chuva` → status de chuva
- LEDs piscam quando chuva detectada.

## Hardware Utilizado

- ESP32
- Sensor DHT11 (GPIO 32)
- Sensor de chuva analógico (GPIO 34)
- LEDs verde (GPIO 15) e vermelho (GPIO 16)
- Buzzer (GPIO 2)
- Display SSD1306 I2C (SDA 21, SCL 22)

## Configurações

- **WiFi**: SSID `******`, senha `******`
- **Broker MQTT**: `broker.hivemq.com`, porta `1883`, client ID `EST_01`

## Uso

1. Conecte o ESP32 aos sensores e display conforme os pinos configurados.
2. Faça upload do código para o ESP32 usando o Arduino IDE.
3. Abra o Serial Monitor para acompanhar a inicialização, leituras e status MQTT.
4. Monitore os tópicos MQTT para receber dados da estação em tempo real.

---

Feito como projeto escolar, destinado a demonstração de sensores, MQTT e visualização em display OLED.
