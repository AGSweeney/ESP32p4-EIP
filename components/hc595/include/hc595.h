#ifndef HC595_H
#define HC595_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int gpio_data;
    int gpio_clock;
    int gpio_latch;
    int gpio_oe;      /* optional, set to -1 if unused */
    int gpio_clear;   /* optional, set to -1 if unused */
    bool clock_idle_high;
} hc595_config_t;

typedef struct {
    hc595_config_t config;
} hc595_t;

esp_err_t hc595_init(hc595_t *dev, const hc595_config_t *cfg);
esp_err_t hc595_shift_byte(hc595_t *dev, uint8_t value);
esp_err_t hc595_shift_buffer(hc595_t *dev, const uint8_t *data, size_t length);
esp_err_t hc595_set_output_enable(hc595_t *dev, bool enable);
esp_err_t hc595_clear(hc595_t *dev);

#ifdef __cplusplus
}
#endif

#endif

