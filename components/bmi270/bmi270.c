#include "bmi270.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static esp_err_t write_then_read(i2c_master_dev_handle_t handle, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    return i2c_master_transmit_receive(handle, tx, tx_len, rx, rx_len, pdMS_TO_TICKS(100));
}

bool bmi270_init(bmi270_t *dev, i2c_master_dev_handle_t i2c_dev)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;
    return true;
}

esp_err_t bmi270_write_register(bmi270_t *dev, uint8_t reg, uint8_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

esp_err_t bmi270_read_register(bmi270_t *dev, uint8_t reg, uint8_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return write_then_read(dev->i2c_dev, &reg, 1, value, 1);
}

esp_err_t bmi270_read_bytes(bmi270_t *dev, uint8_t reg, uint8_t *buffer, size_t length)
{
    if (!dev || !buffer || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return write_then_read(dev->i2c_dev, &reg, 1, buffer, length);
}

static esp_err_t modify_register(bmi270_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current = 0;
    esp_err_t err = bmi270_read_register(dev, reg, &current);
    if (err != ESP_OK)
    {
        return err;
    }
    current = (current & ~mask) | (value & mask);
    return bmi270_write_register(dev, reg, current);
}

esp_err_t bmi270_configure_default(bmi270_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = bmi270_soft_reset(dev);
    if (err != ESP_OK)
    {
        return err;
    }
    err = bmi270_enable_sensors(dev, true, true);
    if (err != ESP_OK)
    {
        return err;
    }
    err = bmi270_set_accel_config(dev, 0x08, 0x00, 0x02);
    if (err != ESP_OK)
    {
        return err;
    }
    return bmi270_set_gyro_config(dev, 0x08, 0x00, 0x02);
}

esp_err_t bmi270_soft_reset(bmi270_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = bmi270_write_register(dev, BMI270_REG_CMD, BMI270_CMD_SOFT_RESET);
    if (err != ESP_OK)
    {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
    return ESP_OK;
}

esp_err_t bmi270_read_chip_id(bmi270_t *dev, uint8_t *chip_id)
{
    return bmi270_read_register(dev, BMI270_REG_CHIP_ID, chip_id);
}

esp_err_t bmi270_set_accel_config(bmi270_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t conf = (uint8_t)((odr_bits & 0x0F) | ((filter_bits & 0x07) << 4));
    esp_err_t err = bmi270_write_register(dev, BMI270_REG_ACC_CONF, conf);
    if (err != ESP_OK)
    {
        return err;
    }
    return bmi270_write_register(dev, BMI270_REG_ACC_RANGE, (uint8_t)(range_bits & 0x07));
}

esp_err_t bmi270_set_gyro_config(bmi270_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t conf = (uint8_t)((odr_bits & 0x0F) | ((filter_bits & 0x07) << 4));
    esp_err_t err = bmi270_write_register(dev, BMI270_REG_GYR_CONF, conf);
    if (err != ESP_OK)
    {
        return err;
    }
    return bmi270_write_register(dev, BMI270_REG_GYR_RANGE, (uint8_t)(range_bits & 0x07));
}

esp_err_t bmi270_enable_sensors(bmi270_t *dev, bool accel_enable, bool gyro_enable)
{
    uint8_t mask = BMI270_PWR_CTRL_ACC_EN | BMI270_PWR_CTRL_GYR_EN;
    uint8_t value = 0;
    if (accel_enable)
    {
        value |= BMI270_PWR_CTRL_ACC_EN;
    }
    if (gyro_enable)
    {
        value |= BMI270_PWR_CTRL_GYR_EN;
    }
    return modify_register(dev, BMI270_REG_PWR_CTRL, mask, value);
}

esp_err_t bmi270_read_sample(bmi270_t *dev, bmi270_sample_t *sample)
{
    if (!dev || !sample)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t buffer[14] = {0};
    esp_err_t err = bmi270_read_bytes(dev, BMI270_REG_GYR_DATA, buffer, sizeof(buffer));
    if (err != ESP_OK)
    {
        return err;
    }
    sample->gyro_x = (int16_t)((buffer[1] << 8) | buffer[0]);
    sample->gyro_y = (int16_t)((buffer[3] << 8) | buffer[2]);
    sample->gyro_z = (int16_t)((buffer[5] << 8) | buffer[4]);
    sample->accel_x = (int16_t)((buffer[7] << 8) | buffer[6]);
    sample->accel_y = (int16_t)((buffer[9] << 8) | buffer[8]);
    sample->accel_z = (int16_t)((buffer[11] << 8) | buffer[10]);
    sample->temperature = (int16_t)((buffer[13] << 8) | buffer[12]);
    return ESP_OK;
}

#define DEG_TO_RAD (3.14159265358979323846f / 180.0f)
#define TWO_PI 6.28318530717958647692f

void bmi270_orientation_init(bmi270_orientation_state_t *state, float alpha)
{
    if (!state)
    {
        return;
    }
    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }
    if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }
    state->roll = 0.0f;
    state->pitch = 0.0f;
    state->yaw = 0.0f;
    state->alpha = alpha;
    state->initialized = false;
}

static float wrap_angle(float angle)
{
    const float pi = 3.14159265358979323846f;
    while (angle > pi)
    {
        angle -= TWO_PI;
    }
    while (angle < -pi)
    {
        angle += TWO_PI;
    }
    return angle;
}

esp_err_t bmi270_orientation_update(bmi270_orientation_state_t *state,
                                    const bmi270_sample_t *sample,
                                    float gyro_dps_per_lsb,
                                    float accel_g_per_lsb,
                                    float dt_seconds,
                                    bmi270_euler_t *euler)
{
    if (!state || !sample || !euler || dt_seconds <= 0.0f)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const float gyro_scale = gyro_dps_per_lsb * DEG_TO_RAD;
    const float ax = sample->accel_x * accel_g_per_lsb;
    const float ay = sample->accel_y * accel_g_per_lsb;
    const float az = sample->accel_z * accel_g_per_lsb;

    const float accel_roll = atan2f(ay, az);
    const float accel_pitch = atan2f(-ax, sqrtf(ay * ay + az * az));

    const float gyro_roll_rate = sample->gyro_x * gyro_scale;
    const float gyro_pitch_rate = sample->gyro_y * gyro_scale;
    const float gyro_yaw_rate = sample->gyro_z * gyro_scale;

    if (!state->initialized)
    {
        state->roll = accel_roll;
        state->pitch = accel_pitch;
        state->yaw = 0.0f;
        state->initialized = true;
    }

    state->roll = state->alpha * (state->roll + gyro_roll_rate * dt_seconds) +
                  (1.0f - state->alpha) * accel_roll;
    state->pitch = state->alpha * (state->pitch + gyro_pitch_rate * dt_seconds) +
                   (1.0f - state->alpha) * accel_pitch;
    state->yaw = wrap_angle(state->yaw + gyro_yaw_rate * dt_seconds);

    euler->roll = state->roll;
    euler->pitch = state->pitch;
    euler->yaw = state->yaw;
    return ESP_OK;
}

