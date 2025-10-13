#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "lib/noise_sensor_lib/SensorHub.h"

static const char *TAG = "SensorHubExample";

// I2C configuration
#define I2C_MASTER_SCL_IO          22
#define I2C_MASTER_SDA_IO          21
#define I2C_MASTER_NUM             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ         100000
#define I2C_MASTER_TX_BUF_DISABLE  0
#define I2C_MASTER_RX_BUF_DISABLE  0

extern "C" void app_main(void)
{
    // --- Initialize I2C driver ---
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                                       I2C_MASTER_RX_BUF_DISABLE,
                                       I2C_MASTER_TX_BUF_DISABLE, 0));

    ESP_LOGI(TAG, "I2C initialized");

    // --- Initialize SensorHub instance ---
    uint8_t sensor_address = 0x40; // Replace with actual address
    SensorHub sensor(sensor_address);

    // --- Check if the sensor is connected ---
    if (sensor.is_sensor_connected()) {
        ESP_LOGI(TAG, "Sensor detected at address 0x%02X", sensor_address);
    } else {
        ESP_LOGE(TAG, "Sensor not detected at address 0x%02X", sensor_address);
    }

    // --- Main loop ---
    uint8_t data = 0;
    while (true) {
        bool ok = sensor.i2c_readByte(0x00, &data, 1);
        if (ok) {
            ESP_LOGI(TAG, "Register 0x00 = 0x%02X (%u)", data, data);
        } else {
            ESP_LOGW(TAG, "Failed to read register 0x00");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
