#include "BH1750.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "BH1750";

static i2c_port_t bh_port = I2C_NUM_0;
static uint8_t bh_addr = BH1750_ADDR_LOW;

esp_err_t BH1750_init(i2c_port_t port, uint8_t addr, int scl_pin, int dat_pin) {
    bh_port = port;
    bh_addr = addr;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = dat_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    ESP_ERROR_CHECK(i2c_param_config(bh_port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(bh_port, conf.mode, 0, 0, 0));

    uint8_t power_on_cmd = 0x01;
    esp_err_t err = i2c_master_write_to_device(bh_port, bh_addr, &power_on_cmd, 1, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power on sensor: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t mode_cmd = BH1750_CONT_HI_RES_MODE;
    err = i2c_master_write_to_device(bh_port, bh_addr, &mode_cmd, 1, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set mode: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "BH1750 initialized at 0x%02X", bh_addr);
    return ESP_OK;
}

esp_err_t BH1750_read_lux(float *lux) {
    if (!lux) return ESP_ERR_INVALID_ARG;

    uint8_t data[2];
    esp_err_t err = i2c_master_read_from_device(bh_port, bh_addr, data, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *lux = raw / 1.2f;

    return ESP_OK;
}
