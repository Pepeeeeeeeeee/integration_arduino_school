#include <Arduino.h>

#include "APIClient.h"
#include "BH1750.h"
#include "INMP441_arduino.h"
#include "DHT11.h"

// WIFI CREDENTIALS
static String WIFI_SSID  = "";
static String WIFI_PASS  = "";
static String API_URL    = "http://192.168.1.33:8000/v1/ingest";
static String API_KEY    = "REDACTED-API-KEY";
static String DEVICE_UID = "esp32-1";

// Send every 1 minute
static const uint32_t SEND_INTERVAL_MS = 1UL * 60UL * 1000UL;
// Wi-Fi connect attempt window
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
// Backoff after a failed send (caps at MAX_BACKOFF_MS)
static const uint32_t BASE_BACKOFF_MS = 2000;
static const uint32_t MAX_BACKOFF_MS  = 60000;

uint32_t nextSendAt = 0;
uint32_t backoffMs = BASE_BACKOFF_MS;


// SENSOR DEFINES
#define DHT_PIN 5

#define I2S_WS_PIN  25
#define I2S_SCK_PIN 26
#define I2S_SD_PIN  33

#define BH1750_SCL_PIN 22
#define BH1750_DAT_PIN 21

void setupSensors(){
  if (DHT11_init((gpio_num_t)DHT_PIN) != ESP_OK) {
    Serial.println("DHT11 init failed!");
  }

  if (BH1750_init(I2C_NUM_0, BH1750_ADDR_LOW, BH1750_SCL_PIN, BH1750_DAT_PIN) != ESP_OK) {
    Serial.println("BH1750 init failed!");
  }

  if (!INMP441_begin(I2S_WS_PIN, I2S_SCK_PIN, I2S_SD_PIN)) {
    Serial.println("INMP441 init failed!");
  }
}



void setup() {
  Serial.begin(115200);
  delay(250);

  printMacAddress();

  APIClient client = {
    .wifiSsid = WIFI_SSID,
    .wifiPass = WIFI_PASS,
    .apiUrl   = API_URL,
    .apiKey   = API_KEY,
    .deviceUid= DEVICE_UID,
    .wifiConnectTimeoutMs = WIFI_CONNECT_TIMEOUT_MS
  };

  APIClient_init(client);

  setupSensors();

  nextSendAt = millis() + 5000; // First send after 5 seconds
  Serial.println("Setup complete.");
}

void loop() {
  const uint32_t now = millis();

  if (now >= nextSendAt) {
    float lux = 0.0f;
    DHT11_Data data;
    Serial.println("Taking measurements...");
    if (BH1750_read_lux(&lux) == ESP_OK) {
      Serial.printf("Light: %.2f lx\n", lux);
    } else {
      Serial.println("Failed to read from BH1750");
    }
    
    if (DHT11_read(&data) == ESP_OK) {
      Serial.printf("Temp: %.1f°C  Humidity: %.1f%%\n", data.temperature, data.humidity);
    } else {
      Serial.println("Failed to read DHT11");
    }

    float dB = INMP441_read_level();
    if (dB == 0) {
      Serial.println("Failed to read from INMP441");
    } else {
      Serial.printf("Sound dB: %.2f\n", dB);
    }

    Serial.println("===========================");
    Serial.println("Sending measurements...");

    Measurement measurement = {
      .temperatureC = data.temperature,
      .humidityPct  = data.humidity,
      .lightLux     = lux,
      .noiseDb      = dB
    };
   
    if (sendMeasurementOnce(&measurement)) {
      backoffMs = BASE_BACKOFF_MS;
      nextSendAt = now + SEND_INTERVAL_MS;
    } else {
      nextSendAt = now + backoffMs;
      backoffMs = min(backoffMs * 2, MAX_BACKOFF_MS);
      Serial.printf("Send failed, backing off to %lu ms\n", backoffMs);
    }

    Serial.println("Next send in " + String(nextSendAt - now) + " ms");
  }

  delay(50);
}


