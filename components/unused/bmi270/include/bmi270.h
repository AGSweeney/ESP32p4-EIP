#ifndef BMI270_H
#define BMI270_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BMI270_I2C_ADDR_PRIMARY 0x68
#define BMI270_I2C_ADDR_SECONDARY 0x69

#define BMI270_REG_CHIP_ID 0x00
#define BMI270_REG_STATUS 0x03
#define BMI270_REG_ACC_DATA 0x12
#define BMI270_REG_GYR_DATA 0x0C
#define BMI270_REG_TEMP_DATA 0x22
#define BMI270_REG_ACC_CONF 0x40
#define BMI270_REG_ACC_RANGE 0x41
#define BMI270_REG_GYR_CONF 0x42
#define BMI270_REG_GYR_RANGE 0x43
#define BMI270_REG_PWR_CONF 0x7C
#define BMI270_REG_PWR_CTRL 0x7D
#define BMI270_REG_CMD 0x7E

#define BMI270_CMD_SOFT_RESET 0xB6
#define BMI270_PWR_CTRL_ACC_EN 0x04
#define BMI270_PWR_CTRL_GYR_EN 0x02

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;
} bmi270_sample_t;

typedef struct {
    float roll;
    float pitch;
    float yaw;
} bmi270_euler_t;

typedef struct {
    float roll;
    float pitch;
    float yaw;
    float alpha;
    bool initialized;
} bmi270_orientation_state_t;

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} bmi270_t;

bool bmi270_init(bmi270_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t bmi270_write_register(bmi270_t *dev, uint8_t reg, uint8_t value);
esp_err_t bmi270_read_register(bmi270_t *dev, uint8_t reg, uint8_t *value);
esp_err_t bmi270_read_bytes(bmi270_t *dev, uint8_t reg, uint8_t *buffer, size_t length);
esp_err_t bmi270_configure_default(bmi270_t *dev);
esp_err_t bmi270_soft_reset(bmi270_t *dev);
esp_err_t bmi270_read_chip_id(bmi270_t *dev, uint8_t *chip_id);
esp_err_t bmi270_set_accel_config(bmi270_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t bmi270_set_gyro_config(bmi270_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t bmi270_enable_sensors(bmi270_t *dev, bool accel_enable, bool gyro_enable);
esp_err_t bmi270_read_sample(bmi270_t *dev, bmi270_sample_t *sample);
void bmi270_orientation_init(bmi270_orientation_state_t *state, float alpha);
esp_err_t bmi270_orientation_update(bmi270_orientation_state_t *state,
                                    const bmi270_sample_t *sample,
                                    float gyro_dps_per_lsb,
                                    float accel_g_per_lsb,
                                    float dt_seconds,
                                    bmi270_euler_t *euler);

#ifdef __cplusplus
}
#endif

#endif

