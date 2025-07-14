# Embarcados-IoT
# Projeto ESP32 - Monitoramento e Controle RGB via MQTT

Este projeto IoT com ESP32 integra sensores e atuadores com comunicação MQTT para monitoramento de temperatura, umidade e controle de LED RGB com modos e cores definidos.

## Requisitos de Hardware

* ESP32 DevKit V1
* Sensor DHT22
* Botão pushbutton
* Anel NeoPixel (24 LEDs RGB)
* Jumpers e protoboard

## Conexões de Hardware

| Componente | GPIO (ESP32) | Observação           |
| ---------- | ------------ | -------------------- |
| DHT22      | GPIO 23      | Pino de dados        |
| NeoPixel   | GPIO 17      | Entrada de dados DIN |
| Botão      | GPIO 21      | Usando INPUT\_PULLUP |

> Todos os VCC conectados ao 3V3 e GND ao GND do ESP32.

## Como compilar e testar

### 1. Ambiente de desenvolvimento

Use a IDE Arduino com as seguintes configurações:

* Placa: "ESP32 Dev Module"
* Velocidade: 115200
* Biblioteca adicionais:

  * `DHT sensor library` (Adafruit)
  * `Adafruit NeoPixel`
  * `PubSubClient`

### 2. Instalação de bibliotecas

Abra o **Gerenciador de Bibliotecas** e instale:

* `DHT sensor library by Adafruit`
* `Adafruit NeoPixel`
* `PubSubClient by Nick O'Leary`

### 3. Rede Wi-Fi e MQTT

O dispositivo se conecta a:

* Wi-Fi SSID: `BAD BOYS`
* Senha: `22780694`
* Broker MQTT: `broker.hivemq.com`

> Certifique-se de que a rede é 2.4GHz, pois o ESP32 não suporta 5GHz.

### 4. Upload do sketch

* Copie o código principal para o arquivo `.ino`
* Conecte o ESP32 via USB
* Selecione a porta correta
* Clique em **Upload**

### 5. Monitoramento

* Abra o **Serial Monitor** (115200 baud rate)
* Observe a conexão Wi-Fi e publicação de JSON
* Pressione o botão:

  * Pressão curta: alterna o modo do LED
  * Pressão longa (> 3s): muda a cor

### 6. Comandos via MQTT

Envie comandos para `/desafio/comando`:

* Mudar modo:

  ```
  mode:2
  ```
* Mudar cor:

  ```
  color:255,0,0
  ```

### 7. JSON de status

Publica a cada 3 segundos em `/desafio/status`:

```json
{"temp":26.5, "hum":60.0, "mode":2, "color":16711680}
```

## Funções principais

| Tarefa         | Função         | Core |
| -------------- | -------------- | ---- |
| LED RGB        | `ledTask()`    | 1    |
| Sensor DHT22   | `sensorTask()` | 1    |
| Botão          | `buttonTask()` | 0    |
| Relatório MQTT | `reportTask()` | 0    |

## Observações

* O projeto é compatível com Wokwi, desde que o Wokwi Gateway esteja ativo na máquina local
* Utilize apenas redes 2.4GHz
* Caso deseje simular, comente os blocos `WiFi.begin()` e `mqtt.connect()` e use apenas o `Serial.println(json)`

---

> Desenvolvido como parte de um desafio técnico em sistemas embarcados e IoT.
>
> <img width="1135" height="769" alt="image" src="https://github.com/user-attachments/assets/06223679-a91d-46db-acab-76d7f9c7b03a" />

