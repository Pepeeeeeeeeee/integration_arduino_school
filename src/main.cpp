#include <Arduino.h>
#include "BH1750.h"
#include "INMP441_arduino.h"
#include "DHT11.h"
#include "MeasurementBuffer.h"

// WIFI CREDENTIALS
static String WIFI_SSID  = "REDACTED-NETWORK-NAME";
static String WIFI_PASS  = ""; // individual password
static String API_URL    = "http://10.134.178.156:8000/v1/ingest";

static String API_KEY    = "REDACTED-API-KEY";
static String DEVICE_UID = "ESP_TEST";

static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000; // Wi-Fi connect attempt window
static const uint32_t SEND_INTERVAL_MS = 5UL * 60UL * 1000UL; // Send every 5 minutes 

// NTP settings
const char* NTP_SERVER = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 3600; // For CET (Brussels)
const int   DAYLIGHT_OFFSET_SEC = 3600;

// BUFFER
#define MEASUREMENT_BUFFER_CAPACITY 100

static MeasurementBuffer measurementBuffer;
static bool timeSynced = false;

// SENSORS
#define DHT_PIN 5

#define I2S_WS_PIN  25
#define I2S_SCK_PIN 26
#define I2S_SD_PIN  33

#define BH1750_SCL_PIN 22
#define BH1750_DAT_PIN 21

static const uint32_t SENSOR_RETRY_WINDOW_MS = 10000;  // Retry window
static const uint32_t SENSOR_RETRY_DELAY_MS  = 250;   // delay between retries
static const uint32_t SENSOR_READ_INTERVAL_MS = 1000; // Read every second for averaging

uint32_t nextSendAt = 5000; // First send after 5 seconds
uint32_t nextReadAt = 2500; // First read after 2.5 seconds


void setupTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        timeSynced = true;
        Serial.println("[Time] NTP time synced successfully.");
    } else {
        Serial.println("[Time] NTP time sync failed.");
    }
}

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

  measurementBuffer.begin(MEASUREMENT_BUFFER_CAPACITY);

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
  setupTime();
  setupSensors();

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

void flushBuffer() {
    Serial.printf("[Flush] %u measurements in buffer. Flushing...\n", measurementBuffer.getCount());
    Measurement m;
    while (measurementBuffer.peek(m)) {
        Serial.printf("[Flush] Sending measurement from %ld\n", m.timestamp);
        if (sendMeasurementOnce(&m)) {
            measurementBuffer.pop(m); // Remove from buffer on success
        } 
        else {
            Serial.println("[Flush] Send failed. Stopping flush.");
            return; // Stop flushing on failure
        }
    }
    Serial.println("[Flush] Buffer flushed successfully.");
}

const uint8_t readingsBufferSize = SEND_INTERVAL_MS / SENSOR_READ_INTERVAL_MS;
float tempBuffer[readingsBufferSize];
float humBuffer[readingsBufferSize];
float luxBuffer[readingsBufferSize];
float noiseBuffer[readingsBufferSize];
uint8_t currentReadingIndex = 0;

void loop() {
  const uint32_t now = millis();

  if (now >= nextReadAt) {
    DHT11_Data dhtData;
    float lux = 0.0f;
    float noiseDb = 0.0f;

    bool dhtOk = readDHT11_with_retry(&dhtData);
    bool bhOk  = readBH1750_with_retry(&lux);
    bool inmpOk= readINMP441_with_retry(&noiseDb);

    if (dhtOk && bhOk && inmpOk) {
        tempBuffer[currentReadingIndex] = dhtData.temperature;
        humBuffer[currentReadingIndex]  = dhtData.humidity;
        luxBuffer[currentReadingIndex]  = lux;
        noiseBuffer[currentReadingIndex]= noiseDb;

        Serial.printf("[Reading %d] Temp=%.2f C, Hum=%.2f %%, Lux=%.2f lx, Noise=%.2f dB\n", 
                      currentReadingIndex, dhtData.temperature, dhtData.humidity, lux, noiseDb);

        currentReadingIndex = (currentReadingIndex + 1) % readingsBufferSize;
    } else {
        Serial.println("[Read] One or more sensor reads failed, skipping this reading.");
    }

    nextReadAt = now + SENSOR_READ_INTERVAL_MS;
    return;
  }

  if (now >= nextSendAt) {
    if (!timeSynced) {
        setupTime(); // Try to re-sync time if it failed
    }

    if (!timeSynced) {
        Serial.println("[Loop] Cannot take new measurement, time is not synced.");
        nextSendAt = now + SEND_INTERVAL_MS;
        return;
    }

    float tempAvg = 0.0f;
    float humAvg  = 0.0f;
    float luxAvg  = 0.0f;
    float noiseAvg= 0.0f;

    for (int i = 0; i < readingsBufferSize; ++i) {
        tempAvg += tempBuffer[i];
        humAvg += humBuffer[i];
        luxAvg += luxBuffer[i];
        noiseAvg += noiseBuffer[i];

        Serial.printf("[Averaging] Reading %d: Temp=%.2f C, Hum=%.2f %%, Lux=%.2f lx, Noise=%.2f dB\n", 
                      i, tempBuffer[i], humBuffer[i], luxBuffer[i], noiseBuffer[i]);
    }

    tempAvg /= readingsBufferSize;
    humAvg /= readingsBufferSize;
    luxAvg /= readingsBufferSize;
    noiseAvg /= readingsBufferSize;

    Measurement measurement = {
      .temperatureC = tempAvg,
      .humidityPct  = humAvg,
      .lightLux     = luxAvg,
      .noiseDb      = noiseAvg,
      .timestamp    =  0
    };

    if (sendMeasurementOnce(&measurement)) {
      // After a successful send, try to flush the buffer
      if (!measurementBuffer.isEmpty()) {
        flushBuffer();
      }
    } 
    else {
      Serial.println("Send failed, attempting to buffer measurement.");
      measurement.timestamp = time(nullptr);
      if (measurementBuffer.push(measurement)) {
          Serial.println("Measurement buffered successfully.");
    } else {
          Serial.println("Buffer is full, measurement lost!");
      }
    }

    nextSendAt = now + SEND_INTERVAL_MS;
    Serial.println("Next send in " + String(nextSendAt - now) + " ms");
    Serial.println("============================================");
  }

  delay(50);
}