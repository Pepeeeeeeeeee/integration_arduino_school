#pragma once
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperature;
    float humidity;
} DHT11_Data;

esp_err_t DHT11_init(gpio_num_t pin);
esp_err_t DHT11_read(DHT11_Data *out);

#ifdef __cplusplus
}
#endif
