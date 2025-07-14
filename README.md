# Embarcados-IoT
Desafio Técnico - Embarcados/IoT com ESP32
Este projeto foi desenvolvido como parte do desafio técnico de embarcados/IoT, utilizando o microcontrolador ESP32 e o framework Arduino. O sistema é composto por um sensor DHT22 para medir temperatura e umidade, um LED RGB Neopixel e um botão para interação.

Objetivo do Projeto
O objetivo deste projeto é implementar um sistema que:

Monitora a temperatura e umidade.
Controla um LED RGB com diferentes modos de operação.
Permite a interação através de um botão.
Envia dados para um broker MQTT em formato JSON.
Recebe comandos via MQTT para alterar o estado do LED.
Hardware Necessário
Microcontrolador ESP32 (preferencialmente S3)
Sensor DHT22 de temperatura e umidade
LED RGB Neopixel
Pushbutton (botão)
Fios de conexão
Configurações de Hardware
DHT22: Conectado ao pino 23 do ESP32.
Botão: Conectado ao pino 21 do ESP32 (configurado com pull-up).
LED RGB: Conectado ao pino 17 do ESP32.
Configurações de Software
Bibliotecas Necessárias
Arduino.h
DHT.h
Adafruit_NeoPixel.h
WiFi.h
PubSubClient.h
Configurações de MQTT/WiFi
SSID: BAD BOYS
Senha: 22780694
Broker MQTT: broker.hivemq.com
Porta MQTT: 1883
Tópico de Status: /desafio/status
Tópico de Comando: /desafio/comando
Tarefas Implementadas
Gerenciamento do LED RGB: Controla o estado e a cor do LED.
Gerenciamento do Sensor de Temperatura e Umidade: Lê e atualiza os valores de temperatura e umidade.
Gerenciamento do Pushbutton: Detecta pressionamentos rápidos e longos para alterar o estado do LED.
Relatório de Estados: Envia dados de temperatura, umidade e estado do LED a cada 3 segundos via MQTT.
Como Compilar e Testar
Clone o Repositório:

bash

Run
Copy code
git clone <link-do-repositorio>
cd <nome-do-repositorio>
Abra o Projeto no Arduino IDE:

Certifique-se de que as bibliotecas necessárias estão instaladas.
Abra o arquivo main.ino ou sketch.ino.
Configuração do Ambiente:

Conecte o ESP32 ao computador.
Selecione a placa correta no Arduino IDE.
Carregar o Código:

Clique em "Upload" para compilar e carregar o código no ESP32.
Monitor Serial:

Abra o Monitor Serial (115200 bps) para visualizar os dados de conexão e os relatórios de estado.
Notas Finais
Certifique-se de que o circuito está montado corretamente conforme a imagem do circuito.
O código foi testado no ambiente Wokwi, mas recomenda-se testar em um dispositivo físico para garantir a funcionalidade completa.
