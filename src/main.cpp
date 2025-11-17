#include <Arduino.h>

#include "APIClient.h"
#include "BH1750.h"
#include "INMP441_arduino.h"
#include "DHT11.h"

// WIFI CREDENTIALS
static String WIFI_SSID  = "REDACTED-NETWORK-NAME";
static String WIFI_PASS  = ""; // individual password
static String API_URL    = "http://10.134.178.156:8000/v1/ingest";

static String API_KEY    = "REDACTED-API-KEY";
static String DEVICE_UID = "ESP_TEST";

// Send every 5 minutes
static const uint32_t SEND_INTERVAL_MS = 5UL * 60UL * 1000UL;

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

static const uint32_t SENSOR_RETRY_WINDOW_MS = 10000;  // try for up to 10 seconds
static const uint32_t SENSOR_RETRY_DELAY_MS  = 250;   // delay between retries

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

bool readBH1750_with_retry(float *lux) {
  const uint32_t start = millis();

  while (millis() - start < SENSOR_RETRY_WINDOW_MS) {
    esp_err_t res = BH1750_read_lux(lux);
    if (res == ESP_OK) {
      Serial.println("BH1750 read OK");
      return true;
    }

    Serial.println("BH1750 read failed, retrying...");
    delay(SENSOR_RETRY_DELAY_MS);
  }

  Serial.println("BH1750 read failed after retries, giving up");
  return false;
}

// Retry wrapper for DHT11
bool readDHT11_with_retry(DHT11_Data *data) {
  const uint32_t start = millis();

  while (millis() - start < SENSOR_RETRY_WINDOW_MS) {
    esp_err_t res = DHT11_read(data);
    if (res == ESP_OK) {
      Serial.println("DHT11 read OK");
      return true;
    }

    Serial.println("DHT11 read failed, retrying...");
    delay(SENSOR_RETRY_DELAY_MS);
  }

  Serial.println("DHT11 read failed after retries, giving up");
  return false;
}

// Retry wrapper for INMP441
bool readINMP441_with_retry(float *dB) {
  const uint32_t start = millis();

  while (millis() - start < SENSOR_RETRY_WINDOW_MS) {
    float level = INMP441_read_level();
    // In your original code, 0 meant failure
    if (level != 0.0f) {
      *dB = level;
      Serial.println("INMP441 read OK");
      return true;
    }

    Serial.println("INMP441 read failed, retrying...");
    delay(SENSOR_RETRY_DELAY_MS);
  }

  Serial.println("INMP441 read failed after retries, giving up");
  return false;
}


void loop() {
  const uint32_t now = millis();

  if (now >= nextSendAt) {
    float lux = 0.0f;
    DHT11_Data dhtData = {};
    float dB = 0.0f;

    Serial.println("Taking measurements...");

    // BH1750
    bool luxOk = readBH1750_with_retry(&lux);
    if (luxOk) {
      Serial.printf("Light: %.2f lx\n", lux);
    } else {
      Serial.println("BH1750 final read failed, using last attempt value (likely 0)");
    }

    // DHT11
    bool dhtOk = readDHT11_with_retry(&dhtData);
    if (dhtOk) {
      Serial.printf("Temp: %.1f°C  Humidity: %.1f%%\n", dhtData.temperature, dhtData.humidity);
    } else {
      Serial.println("DHT11 final read failed, using last attempt values (likely 0)");
    }

    // INMP441
    bool dBok = readINMP441_with_retry(&dB);
    if (dBok) {
      Serial.printf("Sound dB: %.2f\n", dB);
    } else {
      Serial.println("INMP441 final read failed, using last attempt value (likely 0)");
    }

    Serial.println("===========================");
    Serial.println("Sending measurements...");

    Measurement measurement = {
      .temperatureC = dhtData.temperature,
      .humidityPct  = dhtData.humidity,
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