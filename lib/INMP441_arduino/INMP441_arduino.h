#ifndef INMP441_ARDUINO_H
#define INMP441_ARDUINO_H

#include <Arduino.h>

bool INMP441_begin(int ws_pin = -1, int sck_pin = -1, int sd_pin = -1);
float INMP441_read_level(void);

#endif
