#include <Arduino.h>
#include "DHT_U.h"           // DHT Unified sensor
#include "Adafruit_Sensor.h" // Adafruit sensor base
#include <Wire.h>
#include "hp_BH1750.h"       // BH1750 light sensor

// ---------- PLACEHOLDER PINS ----------
#define DHTPIN 4          // Replace with your actual DHT pin
#define DHTTYPE DHT22     // Replace with DHT11/DHT22
#define INMP441_I2S_WS 15 // I2S Word Select / LRCK
#define INMP441_I2S_SD 32 // I2S Serial Data
#define INMP441_I2S_SCK 14 // I2S Serial Clock

// ---------- SENSOR OBJECTS ----------
DHT_Unified dht(DHTPIN, DHTTYPE);
DHT_Unified::Temperature tempSensorObj(&dht, 1);
DHT_Unified::Humidity humiditySensorObj(&dht, 2);
Adafruit_Sensor* tempSensor = &tempSensorObj;
Adafruit_Sensor* humiditySensor = &humiditySensorObj;
hp_BH1750 lightSensor; // default constructor

// ---------- MICROPHONE (I2S) ----------
#include <driver/i2s.h>
#define SAMPLE_RATE 16000
#define I2S_NUM I2S_NUM_0
#define I2S_READ_LEN 1024
int16_t i2sReadBuffer[I2S_READ_LEN];

// ---------- FUNCTION TO INIT I2S ----------
void initI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = INMP441_I2S_SCK,
        .ws_io_num = INMP441_I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = INMP441_I2S_SD
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
}

// ---------- SETUP ----------
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing sensors...");

    // DHT
    dht.begin();

    // BH1750
    Wire.begin(); // default SDA/SCL
    if (!lightSensor.begin(0x23)) { // default BH1750 address
        Serial.println("BH1750 not detected!");
    }

    // INMP441
    initI2S();

    Serial.println("Initialization complete!");
}

// ---------- MAIN LOOP ----------
void loop() {
    sensors_event_t tempEvent, humEvent;

    // --- Read DHT ---
    tempSensor->getEvent(&tempEvent);
    humiditySensor->getEvent(&humEvent);

    Serial.println("----- DHT Sensor -----");
    Serial.print("Temperature: "); Serial.print(tempEvent.temperature); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(humEvent.relative_humidity); Serial.println(" %");

    // --- Read BH1750 ---
    float lux = lightSensor.getLux(); // library function
    Serial.println("----- BH1750 Light Sensor -----");
    Serial.print("Light Level: "); Serial.print(lux); Serial.println(" lx");

    // --- Read INMP441 ---
    size_t bytesRead;
    i2s_read(I2S_NUM, i2sReadBuffer, I2S_READ_LEN * sizeof(int16_t), &bytesRead, portMAX_DELAY);

    // Compute simple RMS amplitude
    float rms = 0;
    int samples = bytesRead / sizeof(int16_t);
    for (int i = 0; i < samples; i++) {
        rms += i2sReadBuffer[i] * i2sReadBuffer[i];
    }
    rms = sqrt(rms / samples);
    Serial.println("----- INMP441 Microphone -----");
    Serial.print("RMS Amplitude: "); Serial.println(rms);

    // --- Wait 10 seconds ---
    Serial.println("\nWaiting 10 seconds...\n");
    delay(10000);
}
