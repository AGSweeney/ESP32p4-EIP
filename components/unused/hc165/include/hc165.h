#ifndef HC165_H
#define HC165_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int gpio_data;   /* QH pin */
    int gpio_clock;  /* CP pin */
    int gpio_latch;  /* SH/LD pin (active low) */
    int gpio_oe;     /* optional, set -1 if unused (OE active low) */
    bool clock_idle_high;
    bool sample_on_falling_edge;
} hc165_config_t;

typedef struct {
    hc165_config_t config;
} hc165_t;

esp_err_t hc165_init(hc165_t *dev, const hc165_config_t *cfg);
esp_err_t hc165_shift_byte(hc165_t *dev, uint8_t *value);
esp_err_t hc165_shift_buffer(hc165_t *dev, uint8_t *data, size_t length);
esp_err_t hc165_set_output_enable(hc165_t *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif

