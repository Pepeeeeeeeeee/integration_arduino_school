#include <Arduino.h>
#include "BH1750.h"
#include "INMP441_arduino.h"
#include "DHT11.h"

#define DHT_PIN 4

#define I2S_WS_PIN  15
#define I2S_SCK_PIN 14
#define I2S_SD_PIN  32

#define BH1750_SCL_PIN 22
#define BH1750_DAT_PIN 21

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting test...");

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

void loop() {
  float lux = 0.0f;
  DHT11_Data data;

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


  delay(2000);
}
