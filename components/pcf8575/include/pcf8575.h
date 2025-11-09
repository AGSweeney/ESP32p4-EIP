#ifndef PCF8575_H
#define PCF8575_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PCF8575_I2C_ADDR_DEFAULT 0x20

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} pcf8575_t;

bool pcf8575_init(pcf8575_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t pcf8575_write(pcf8575_t *dev, uint16_t value);
esp_err_t pcf8575_read(pcf8575_t *dev, uint16_t *value);
esp_err_t pcf8575_update_mask(pcf8575_t *dev, uint16_t mask, uint16_t value);
esp_err_t pcf8575_write_pin(pcf8575_t *dev, uint8_t pin, bool level);
esp_err_t pcf8575_read_pin(pcf8575_t *dev, uint8_t pin, bool *level);

#ifdef __cplusplus
}
#endif

#endif

