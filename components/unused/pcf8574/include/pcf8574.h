#ifndef PCF8574_H
#define PCF8574_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PCF8574_I2C_ADDR_DEFAULT 0x20

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} pcf8574_t;

bool pcf8574_init(pcf8574_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t pcf8574_write(pcf8574_t *dev, uint8_t value);
esp_err_t pcf8574_read(pcf8574_t *dev, uint8_t *value);
esp_err_t pcf8574_update_mask(pcf8574_t *dev, uint8_t mask, uint8_t value);
esp_err_t pcf8574_write_pin(pcf8574_t *dev, uint8_t pin, bool level);
esp_err_t pcf8574_read_pin(pcf8574_t *dev, uint8_t pin, bool *level);

#ifdef __cplusplus
}
#endif

#endif

