#include <Arduino.h>
#include "SensorHub.h"

#define I2S_WS   15   // Connect to INMP441 LRCLK
#define I2S_BCLK 14   // Connect to INMP441 BCLK
#define I2S_D    32   // Connect to INMP441 DATA
#define SAMPLE_RATE 16000

SensorHub mic(I2S_WS, I2S_BCLK, I2S_D, SAMPLE_RATE, I2S_CHANNEL_LEFT);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("INMP441 I2S Microphone Test");

    // I2S initialization for ESP32
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_D
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void loop() {
    int32_t sample;
    size_t bytes_read;

    // Read one sample
    i2s_read(I2S_NUM_0, &sample, sizeof(sample), &bytes_read, portMAX_DELAY);

    // Convert to int16_t for simplicity
    int16_t value = sample >> 16;
    Serial.println(value);

    delay(10);
}