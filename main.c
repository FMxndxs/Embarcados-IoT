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

// ------------------- CONFIGURAÇÕES DE MQTT/WIFI -------------------
const char* ssid       = "BAD BOYS";
const char* password   = "22780694";
const char* mqttServer = "broker.hivemq.com";
const int   mqttPort   = 1883;
const char* statusTopic = "/desafio/status";
const char* cmdTopic    = "/desafio/comando";

// ------------------- OBJETOS GLOBAIS -------------------
DHT dht(DHTPIN, DHTTYPE);
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient mqtt(espClient);

float temperature = 0;
float humidity = 0;
uint32_t currentColor = 0;
int ledMode = 0;
SemaphoreHandle_t stateMutex;

uint32_t nextColor(uint32_t color) {
  if (color == pixels.Color(0,255,0)) return pixels.Color(255,255,0);
  if (color == pixels.Color(255,255,0)) return pixels.Color(255,0,0);
  if (color == pixels.Color(255,0,0)) return pixels.Color(0,0,255);
  return pixels.Color(0,255,0);
}

void ledTask(void* pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while(true) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    uint32_t col = currentColor;
    int mode = ledMode;
    xSemaphoreGive(stateMutex);

    switch(mode) {
      case 0: pixels.clear(); pixels.show(); break;
      case 1: pixels.fill(col); pixels.show(); break;
      case 2: pixels.fill(col); pixels.show(); vTaskDelay(pdMS_TO_TICKS(500));
              pixels.clear(); pixels.show(); vTaskDelay(pdMS_TO_TICKS(500)); continue;
      case 3: pixels.fill(col); pixels.show(); vTaskDelay(pdMS_TO_TICKS(150));
              pixels.clear(); pixels.show(); vTaskDelay(pdMS_TO_TICKS(150)); continue;
    }
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
  }
}

void sensorTask(void* pvParameters) {
  while(true) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      xSemaphoreTake(stateMutex, portMAX_DELAY);
      temperature = t;
      humidity = h;
      xSemaphoreGive(stateMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void handleButtonPress(unsigned long duration) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  if(duration < 1000) {
    ledMode = (ledMode + 1) % 4;
  } else if(duration > 3000) {
    currentColor = nextColor(currentColor);
  }
  xSemaphoreGive(stateMutex);
}

volatile unsigned long pressStart = 0;

void IRAM_ATTR onButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    pressStart = millis();
  } else {
    unsigned long duration = millis() - pressStart;
    static BaseType_t xHigherPriorityTaskWoken;
    xTaskCreatePinnedToCore([](void* param) {
      unsigned long dur = *(unsigned long*)param;
      handleButtonPress(dur);
      vTaskDelete(NULL);
    }, "BtnHandler", 2048, (void*)&duration, 2, NULL, 0);
  }
}

void buttonTask(void* pvParameters) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, onButton, CHANGE);
  while(true) vTaskDelay(pdMS_TO_TICKS(10000));
}

void reportTask(void* pvParameters) {
  while(true) {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    float t = temperature;
    float h = humidity;
    uint32_t col = currentColor;
    int mode = ledMode;
    xSemaphoreGive(stateMutex);

    String json = "{\"temp\":" + String(t, 1) + ", \"hum\":" + String(h, 1) + ", \"mode\":" + String(mode) + ", \"color\":" + String(col) + "}";
    Serial.println(json);
    if (mqtt.connected()) {
      mqtt.publish(statusTopic, json.c_str());
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String cmd = "";
  for (int i = 0; i < length; i++) cmd += (char)payload[i];

  xSemaphoreTake(stateMutex, portMAX_DELAY);
  if (cmd.startsWith("mode:")) {
    ledMode = cmd.substring(5).toInt();
  } else if (cmd.startsWith("color:")) {
    int r, g, b;
    sscanf(cmd.substring(6).c_str(), "%d,%d,%d", &r, &g, &b);
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

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wifi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(mqttCallback);
  while (!mqtt.connected()) {
    mqtt.connect("ESP32_Desafio");
    delay(500);
  }
  mqtt.subscribe(cmdTopic);

  xTaskCreatePinnedToCore(ledTask, "LED", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sensorTask, "Sensor", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(buttonTask, "Button", 2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(reportTask, "Report", 4096, NULL, 1, NULL, 0);
}

void loop() {
  if (mqtt.connected()) mqtt.loop();
}
