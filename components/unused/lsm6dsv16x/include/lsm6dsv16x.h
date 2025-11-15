#ifndef LSM6DSV16X_H
#define LSM6DSV16X_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LSM6DSV16X_I2C_ADDR_PRIMARY 0x6A
#define LSM6DSV16X_I2C_ADDR_SECONDARY 0x6B

#define LSM6DSV16X_REG_CTRL1_XL 0x10
#define LSM6DSV16X_REG_CTRL2_G 0x11
#define LSM6DSV16X_REG_CTRL3_C 0x12
#define LSM6DSV16X_REG_WHO_AM_I 0x0F
#define LSM6DSV16X_REG_OUTX_L_G 0x22
#define LSM6DSV16X_REG_OUTX_L_A 0x28

#define LSM6DSV16X_CTRL3_C_SW_RESET 0x01

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} lsm6dsv16x_sample_t;

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} lsm6dsv16x_t;

bool lsm6dsv16x_init(lsm6dsv16x_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t lsm6dsv16x_write_register(lsm6dsv16x_t *dev, uint8_t reg, uint8_t value);
esp_err_t lsm6dsv16x_read_register(lsm6dsv16x_t *dev, uint8_t reg, uint8_t *value);
esp_err_t lsm6dsv16x_read_bytes(lsm6dsv16x_t *dev, uint8_t reg, uint8_t *buffer, size_t length);
esp_err_t lsm6dsv16x_configure_default(lsm6dsv16x_t *dev);
esp_err_t lsm6dsv16x_soft_reset(lsm6dsv16x_t *dev);
esp_err_t lsm6dsv16x_read_id(lsm6dsv16x_t *dev, uint8_t *who_am_i);
esp_err_t lsm6dsv16x_set_accel_config(lsm6dsv16x_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t lsm6dsv16x_set_gyro_config(lsm6dsv16x_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t lsm6dsv16x_read_sample(lsm6dsv16x_t *dev, lsm6dsv16x_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif

