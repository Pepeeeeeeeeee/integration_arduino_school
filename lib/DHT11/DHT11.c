#include "DHT11.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DHT11";
static gpio_num_t dht_pin = -1;

#define DHT_TIMEOUT_US 5000

static inline int wait_for_level(int level, int timeout_us) {
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(dht_pin) != level) {
        if ((esp_timer_get_time() - start) > timeout_us) return -1;
    }
    return esp_timer_get_time() - start;
}

esp_err_t DHT11_init(gpio_num_t pin) {
    dht_pin = pin;
    gpio_set_direction(dht_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_pin, 1);
    gpio_set_pull_mode(dht_pin, GPIO_PULLUP_ONLY);
    return ESP_OK;
}

esp_err_t DHT11_read(DHT11_Data *out) {
    uint8_t data[5] = {0};

    gpio_set_direction(dht_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_pin, 0);
    ets_delay_us(20000);
    gpio_set_level(dht_pin, 1);
    ets_delay_us(30);
    gpio_set_direction(dht_pin, GPIO_MODE_INPUT);

    if (wait_for_level(0, DHT_TIMEOUT_US) < 0) return ESP_FAIL;
    if (wait_for_level(1, DHT_TIMEOUT_US) < 0) return ESP_FAIL;
    if (wait_for_level(0, DHT_TIMEOUT_US) < 0) return ESP_FAIL;

    for (int i = 0; i < 40; i++) {
        if (wait_for_level(1, DHT_TIMEOUT_US) < 0) return ESP_FAIL;
        int high_time = wait_for_level(0, DHT_TIMEOUT_US);
        if (high_time < 0) return ESP_FAIL;

        int byte = i / 8;
        data[byte] <<= 1;
        if (high_time > 45) data[byte] |= 1;
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGW(TAG, "Checksum mismatch: calc=%d recv=%d", checksum, data[4]);
        return ESP_FAIL;
    }

    out->humidity = data[0];
    out->temperature = data[2];
    return ESP_OK;
}
