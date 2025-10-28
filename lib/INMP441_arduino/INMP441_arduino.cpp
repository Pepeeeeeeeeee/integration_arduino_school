#include "INMP441_arduino.h"
#include <I2S.h>
#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 16000
#define SAMPLES 512

#define rms_ref 100000.0f
#define offset 80.0f

bool INMP441_begin(int ws_pin, int sck_pin, int sd_pin) {
    if (!I2S.begin(I2S_PHILIPS_MODE, SAMPLE_RATE, 32)) {
        Serial.println("Failed to initialize I2S");
        return false;
    }
    I2S.setAllPins(sck_pin, ws_pin, sd_pin, I2S_PIN_NO_CHANGE, I2S_PIN_NO_CHANGE);
    Serial.println("INMP441 initialized");
    return true;
}

float INMP441_read_level() {
    size_t buf_bytes = sizeof(int32_t) * SAMPLES;
    int32_t* samples = (int32_t*) malloc(buf_bytes);
    if (!samples) {
        Serial.println("Failed to allocate sample buffer");
        return 0.0f;
    }

    int bytes_read = I2S.read(samples, buf_bytes);
    if (bytes_read <= 0) {
        free(samples);
        return 0.0f;
    }

    int num_samples = bytes_read / sizeof(int32_t);
    double sum_sq = 0.0;
    for (int i = 0; i < num_samples; i++) {
        int32_t sample = samples[i] >> 8;
        sum_sq += (double)sample * sample;
    }

    float rms = 0.0f;
    if (num_samples > 0) {
        rms = sqrt(sum_sq / num_samples);
    }

    float res_dB = 20.0f * log10f(rms / rms_ref) + offset;

    free(samples);
    return res_dB;
}
