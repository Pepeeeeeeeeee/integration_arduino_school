#ifndef INMP441_H
#define INMP441_H

#include "esp_err.h"

esp_err_t INMP441_init(void);
float INMP441_read_level(void);

#endif
