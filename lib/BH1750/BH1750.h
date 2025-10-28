#ifndef BH1750_H
#define BH1750_H

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BH1750_ADDR_LOW  0x23  // ADDR pin LOW
#define BH1750_ADDR_HIGH 0x5C  // ADDR pin HIGH

typedef enum {
    BH1750_CONT_HI_RES_MODE = 0x10,
    BH1750_CONT_HI_RES_MODE_2 = 0x11,
    BH1750_CONT_LOW_RES_MODE = 0x13,
    BH1750_ONE_HI_RES_MODE = 0x20,
    BH1750_ONE_HI_RES_MODE_2 = 0x21,
    BH1750_ONE_LOW_RES_MODE = 0x23
} bh1750_mode_t;

esp_err_t BH1750_init(i2c_port_t port, uint8_t addr, int scl_pin, int dat_pin);
esp_err_t BH1750_read_lux(float *lux);

#ifdef __cplusplus
}
#endif

#endif
