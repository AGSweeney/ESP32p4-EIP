#ifndef NAU7802_H
#define NAU7802_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAU7802_I2C_ADDR_DEFAULT 0x2A

#define NAU7802_REG_PU_CTRL 0x00
#define NAU7802_REG_CTRL1 0x01
#define NAU7802_REG_CTRL2 0x02
#define NAU7802_REG_CTRL3 0x03
#define NAU7802_REG_ADC 0x12
#define NAU7802_REG_ADC_RESULT 0x12
#define NAU7802_REG_ADC_RESULT_MID 0x13
#define NAU7802_REG_ADC_RESULT_LOW 0x14

#define NAU7802_PU_CTRL_PUD 0x01
#define NAU7802_PU_CTRL_PUA 0x02
#define NAU7802_PU_CTRL_PUR 0x04
#define NAU7802_PU_CTRL_CS 0x08
#define NAU7802_PU_CTRL_CRRDY 0x40
#define NAU7802_PU_CTRL_DRDY 0x80

#define NAU7802_CTRL2_CALMOD_INTERNAL 0x10
#define NAU7802_CTRL2_CALMOD_EXTERNAL 0x20
#define NAU7802_CTRL2_CAL_START 0x08

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} nau7802_t;

bool nau7802_init(nau7802_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t nau7802_write_register(nau7802_t *dev, uint8_t reg, uint8_t value);
esp_err_t nau7802_read_register(nau7802_t *dev, uint8_t reg, uint8_t *value);
esp_err_t nau7802_read_conversion(nau7802_t *dev, int32_t *value);
esp_err_t nau7802_soft_reset(nau7802_t *dev);
esp_err_t nau7802_power_up(nau7802_t *dev);
esp_err_t nau7802_power_down(nau7802_t *dev);
esp_err_t nau7802_set_gain(nau7802_t *dev, uint8_t gain);
esp_err_t nau7802_set_sample_rate(nau7802_t *dev, uint8_t rate);
esp_err_t nau7802_calibrate(nau7802_t *dev, bool internal_reference);
esp_err_t nau7802_is_data_ready(nau7802_t *dev, bool *ready);
esp_err_t nau7802_wait_ready(nau7802_t *dev, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif

