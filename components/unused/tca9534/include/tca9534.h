#ifndef TCA9534_H
#define TCA9534_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TCA9534_I2C_ADDR_DEFAULT 0x20

#define TCA9534_REG_INPUT     0x00
#define TCA9534_REG_OUTPUT    0x01
#define TCA9534_REG_POLARITY  0x02
#define TCA9534_REG_CONFIG    0x03

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} tca9534_t;

typedef struct {
    uint8_t direction; /* 1 = input, 0 = output */
    uint8_t polarity;  /* 1 = invert input */
    uint8_t output;    /* Initial output state for output pins */
} tca9534_config_t;

bool tca9534_init(tca9534_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const tca9534_config_t *cfg);

esp_err_t tca9534_write_register(tca9534_t *dev, uint8_t reg, uint8_t value);
esp_err_t tca9534_read_register(tca9534_t *dev, uint8_t reg, uint8_t *value);

esp_err_t tca9534_write_gpio(tca9534_t *dev, uint8_t value);
esp_err_t tca9534_read_gpio(tca9534_t *dev, uint8_t *value);
esp_err_t tca9534_set_direction(tca9534_t *dev, uint8_t mask);
esp_err_t tca9534_set_polarity(tca9534_t *dev, uint8_t mask);
esp_err_t tca9534_update_gpio_mask(tca9534_t *dev, uint8_t mask, uint8_t value);
esp_err_t tca9534_write_pin(tca9534_t *dev, uint8_t pin, bool level);
esp_err_t tca9534_read_pin(tca9534_t *dev, uint8_t pin, bool *level);

#ifdef __cplusplus
}
#endif

#endif

