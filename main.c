#include <Arduino.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ------------------- CONFIGURAÇÕES DE HARDWARE -------------------
#define DHTPIN        23
#define DHTTYPE       DHT22
#define BUTTON_PIN    21
#define LED_PIN       17
#define NUMPIXELS     24

// ------------------- CONFIGURAÇÕES DE REDE -------------------
const char* ssid         = "BAD BOYS";
const char* password     = "22780694";
const char* mqttServer   = "broker.hivemq.com";
const int   mqttPort     = 1883;
const char* statusTopic  = "/desafio/status";
const char* cmdTopic     = "/desafio/comando";

// ------------------- OBJETOS E ESTADO GLOBAL -------------------
DHT dht(DHTPIN, DHTTYPE);
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient mqtt(espClient);

SemaphoreHandle_t stateMutex;
QueueHandle_t buttonQueue;

float temperature = 0.0f;
float humidity    = 0.0f;
uint32_t currentColor = 0;
int ledMode       = 0;

volatile unsigned long lastISRTime = 0;
volatile unsigned long pressStart  = 0;

// ------------------- PROTÓTIPOS -------------------
uint32_t nextColor(uint32_t color);
void handleButtonPress(unsigned long duration);
void ledTask(void* pvParameters);
void sensorTask(void* pvParameters);
void buttonTask(void* pvParameters);
void reportTask(void* pvParameters);
void mqttCallback(char* topic, byte* payload, unsigned int length);

// ------------------- IMPLEMENTAÇÕES -------------------
uint32_t nextColor(uint32_t color) {
  if (color == pixels.Color(0,255,0)) return pixels.Color(255,255,0);
  if (color == pixels.Color(255,255,0)) return pixels.Color(255,0,0);
  if (color == pixels.Color(255,0,0)) return pixels.Color(0,0,255);
  return pixels.Color(0,255,0);
}

void handleButtonPress(unsigned long duration) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  if (duration < 1000) {
    ledMode = (ledMode + 1) % 4;
  } else if (duration > 3000) {
    currentColor = nextColor(currentColor);
  }
  xSemaphoreGive(stateMutex);
}

void IRAM_ATTR onButtonISR() {
  unsigned long now = millis();
  // debounce: ignorar se <50ms desde última ISR
  if (now - lastISRTime < 50) return;
  lastISRTime = now;

  if (digitalRead(BUTTON_PIN) == LOW) {
    pressStart = now;
  } else {
    unsigned long duration = now - pressStart;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(buttonQueue, &duration, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void ledTask(void* pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    uint32_t col = currentColor;
    int mode    = ledMode;
    xSemaphoreGive(stateMutex);

    switch (mode) {
      case 0:
        pixels.clear(); pixels.show();
        break;
      case 1:
        pixels.fill(col); pixels.show();
        break;
      case 2:
        pixels.fill(col); pixels.show();
        vTaskDelay(pdMS_TO_TICKS(500));
        pixels.clear(); pixels.show();
        vTaskDelay(pdMS_TO_TICKS(500));
        continue;
      case 3:
        pixels.fill(col); pixels.show();
        vTaskDelay(pdMS_TO_TICKS(150));
        pixels.clear(); pixels.show();
        vTaskDelay(pdMS_TO_TICKS(150));
        continue;
    }
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
  }
}

void sensorTask(void* pvParameters) {
  while (true) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      temperature = t;
      humidity    = h;
      xSemaphoreGive(stateMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void buttonTask(void* pvParameters) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, onButtonISR, CHANGE);
  unsigned long duration;
  while (true) {
    if (xQueueReceive(buttonQueue, &duration, portMAX_DELAY) == pdTRUE) {
      handleButtonPress(duration);
    }
  }
}

void reportTask(void* pvParameters) {
  char payload[128];
  while (true) {
    // reconexão Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
    // prote��o de estado
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    float t = temperature;
    float h = humidity;
    uint32_t col = currentColor;
    int mode = ledMode;
    xSemaphoreGive(stateMutex);

    // JSON estático
    int len = snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"hum\":%.1f,\"mode\":%d,\"color\":%lu}",
                       t, h, mode, (unsigned long)col);

    Serial.println(payload);

    // reconexão MQTT e publicação
    if (!mqtt.connected()) {
      if (mqtt.connect("ESP32_Desafio")) {
        mqtt.subscribe(cmdTopic);
      }
    }
    mqtt.publish(statusTopic, payload, len);

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char cmd[32] = {0};
  size_t l = min(length, sizeof(cmd) - 1);
  memcpy(cmd, payload, l);

  xSemaphoreTake(stateMutex, portMAX_DELAY);
  if (strncmp(cmd, "mode:", 5) == 0) {
    ledMode = atoi(cmd + 5);
  } else if (strncmp(cmd, "color:", 6) == 0) {
    int r, g, b;
    sscanf(cmd + 6, "%d,%d,%d", &r, &g, &b);
    currentColor = pixels.Color(r, g, b);
  }
  xSemaphoreGive(stateMutex);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pixels.begin();
  currentColor = pixels.Color(0,255,0);

  stateMutex = xSemaphoreCreateMutex();
  buttonQueue = xQueueCreate(10, sizeof(unsigned long));

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(mqttCallback);

  // Cria tarefas FreeRTOS
  xTaskCreatePinnedToCore(ledTask,    "LED",     4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sensorTask, "Sensor",  4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(buttonTask, "Button",  2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(reportTask, "Report",  4096, NULL, 1, NULL, 0);
}

void loop() {
  mqtt.loop();
}
