#ifndef HX711_H
#define HX711_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HX711_GAIN_128 = 1,
    HX711_GAIN_64  = 3,
    HX711_GAIN_32  = 2,
} hx711_gain_t;

typedef struct {
    int gpio_dout;
    int gpio_sck;
    hx711_gain_t gain;
} hx711_config_t;

typedef struct {
    hx711_config_t config;
} hx711_t;

esp_err_t hx711_init(hx711_t *dev, const hx711_config_t *cfg);
esp_err_t hx711_read_raw(hx711_t *dev, int32_t *value);
esp_err_t hx711_set_gain(hx711_t *dev, hx711_gain_t gain);
bool hx711_is_ready(hx711_t *dev);
esp_err_t hx711_wait_ready(hx711_t *dev, uint32_t timeout_ms);
esp_err_t hx711_read_average(hx711_t *dev, uint8_t samples, int32_t *value);
esp_err_t hx711_power_down(hx711_t *dev);
esp_err_t hx711_power_up(hx711_t *dev);

#ifdef __cplusplus
}
#endif

#endif

