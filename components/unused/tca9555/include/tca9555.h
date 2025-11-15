#ifndef TCA9555_H
#define TCA9555_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TCA9555_I2C_ADDR_DEFAULT 0x20

#define TCA9555_REG_INPUT0    0x00
#define TCA9555_REG_INPUT1    0x01
#define TCA9555_REG_OUTPUT0   0x02
#define TCA9555_REG_OUTPUT1   0x03
#define TCA9555_REG_POL0      0x04
#define TCA9555_REG_POL1      0x05
#define TCA9555_REG_CONFIG0   0x06
#define TCA9555_REG_CONFIG1   0x07

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} tca9555_t;

typedef struct {
    uint8_t config0;
    uint8_t config1;
    uint8_t polarity0;
    uint8_t polarity1;
    uint8_t output0;
    uint8_t output1;
} tca9555_config_t;

bool tca9555_init(tca9555_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const tca9555_config_t *cfg);

esp_err_t tca9555_write_register(tca9555_t *dev, uint8_t reg, uint8_t value);
esp_err_t tca9555_read_register(tca9555_t *dev, uint8_t reg, uint8_t *value);
esp_err_t tca9555_write_gpio(tca9555_t *dev, uint16_t value);
esp_err_t tca9555_read_gpio(tca9555_t *dev, uint16_t *value);
esp_err_t tca9555_set_direction(tca9555_t *dev, uint16_t mask);
esp_err_t tca9555_set_polarity(tca9555_t *dev, uint16_t mask);
esp_err_t tca9555_update_gpio_mask(tca9555_t *dev, uint16_t mask, uint16_t value);
esp_err_t tca9555_write_pin(tca9555_t *dev, uint8_t pin, bool level);
esp_err_t tca9555_read_pin(tca9555_t *dev, uint8_t pin, bool *level);

#ifdef __cplusplus
}
#endif

#endif

