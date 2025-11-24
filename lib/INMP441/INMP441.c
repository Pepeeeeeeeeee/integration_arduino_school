#include "INMP441.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include <math.h>
#include <stdlib.h>

static const char *TAG = "INMP441";

#define I2S_PORT        I2S_NUM_0
#define I2S_WS          15 
#define I2S_SCK         14
#define I2S_SD          32
#define SAMPLE_RATE     16000

#define SAMPLES         512
#define rms_ref         100000.0f
#define offset          80.0f

esp_err_t INMP441_init(void) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, &pin_config));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(I2S_PORT));

    ESP_LOGI(TAG, "INMP441 initialized on WS=%d, SCK=%d, SD=%d", I2S_WS, I2S_SCK, I2S_SD);
    return ESP_OK;
}

float INMP441_read_level(void) {
    size_t buf_bytes = sizeof(int32_t) * SAMPLES;
    int32_t *samples = (int32_t *) malloc(buf_bytes);
    if (!samples) {
        ESP_LOGE(TAG, "Failed to allocate sample buffer (%u bytes)", (unsigned)buf_bytes);
        return 0.0f;
    }

    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_PORT, samples, buf_bytes, &bytes_read, pdMS_TO_TICKS(100));
    if (result != ESP_OK || bytes_read == 0) {
        ESP_LOGW(TAG, "I2S read failed: %s (bytes=%u)", esp_err_to_name(result), (unsigned)bytes_read);
        free(samples);
        return 0.0f;
    }

    int samples_read = (int)(bytes_read / sizeof(int32_t));
    if (samples_read <= 0) {
        free(samples);
        return 0.0f;
    }

    double sum_sq = 0.0;
    for (int i = 0; i < samples_read; i++) {
        int32_t sample = samples[i] >> 8;
        double s = (double)sample;
        sum_sq += s * s;
    }

    float rms = 0.0f;
    if (samples_read > 0) {
        rms = (float) sqrt(sum_sq / samples_read);
    }

    printf("Sound level (RMS): %.2f [dB= %2.f] (samples=%d)", rms, 20.0f * log10f(rms / rms_ref) + offset, samples_read);

    float res_dB = 20.0f * log10f(rms / rms_ref) + offset;

    free(samples);
    return rms;
}

