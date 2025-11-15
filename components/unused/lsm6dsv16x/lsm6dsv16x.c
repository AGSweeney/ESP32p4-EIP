#include "lsm6dsv16x.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static esp_err_t write_then_read(i2c_master_dev_handle_t handle, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    return i2c_master_transmit_receive(handle, tx, tx_len, rx, rx_len, pdMS_TO_TICKS(100));
}

bool lsm6dsv16x_init(lsm6dsv16x_t *dev, i2c_master_dev_handle_t i2c_dev)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;
    return true;
}

esp_err_t lsm6dsv16x_write_register(lsm6dsv16x_t *dev, uint8_t reg, uint8_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

esp_err_t lsm6dsv16x_read_register(lsm6dsv16x_t *dev, uint8_t reg, uint8_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return write_then_read(dev->i2c_dev, &reg, 1, value, 1);
}

esp_err_t lsm6dsv16x_read_bytes(lsm6dsv16x_t *dev, uint8_t reg, uint8_t *buffer, size_t length)
{
    if (!dev || !buffer || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return write_then_read(dev->i2c_dev, &reg, 1, buffer, length);
}

static esp_err_t modify_register(lsm6dsv16x_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current = 0;
    esp_err_t err = lsm6dsv16x_read_register(dev, reg, &current);
    if (err != ESP_OK)
    {
        return err;
    }
    current = (current & ~mask) | (value & mask);
    return lsm6dsv16x_write_register(dev, reg, current);
}

esp_err_t lsm6dsv16x_configure_default(lsm6dsv16x_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = lsm6dsv16x_soft_reset(dev);
    if (err != ESP_OK)
    {
        return err;
    }
    err = lsm6dsv16x_set_accel_config(dev, 0x04, 0x00, 0x00);
    if (err != ESP_OK)
    {
        return err;
    }
    return lsm6dsv16x_set_gyro_config(dev, 0x04, 0x00, 0x00);
}

esp_err_t lsm6dsv16x_soft_reset(lsm6dsv16x_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = modify_register(dev, LSM6DSV16X_REG_CTRL3_C, LSM6DSV16X_CTRL3_C_SW_RESET, LSM6DSV16X_CTRL3_C_SW_RESET);
    if (err != ESP_OK)
    {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
    return ESP_OK;
}

esp_err_t lsm6dsv16x_read_id(lsm6dsv16x_t *dev, uint8_t *who_am_i)
{
    return lsm6dsv16x_read_register(dev, LSM6DSV16X_REG_WHO_AM_I, who_am_i);
}

esp_err_t lsm6dsv16x_set_accel_config(lsm6dsv16x_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t value = (uint8_t)(((odr_bits & 0x0F) << 4) | ((range_bits & 0x03) << 2) | (filter_bits & 0x03));
    return lsm6dsv16x_write_register(dev, LSM6DSV16X_REG_CTRL1_XL, value);
}

esp_err_t lsm6dsv16x_set_gyro_config(lsm6dsv16x_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t value = (uint8_t)(((odr_bits & 0x0F) << 4) | ((range_bits & 0x07) << 1) | (filter_bits & 0x01));
    return lsm6dsv16x_write_register(dev, LSM6DSV16X_REG_CTRL2_G, value);
}

esp_err_t lsm6dsv16x_read_sample(lsm6dsv16x_t *dev, lsm6dsv16x_sample_t *sample)
{
    if (!dev || !sample)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t buffer[12] = {0};
    esp_err_t err = lsm6dsv16x_read_bytes(dev, LSM6DSV16X_REG_OUTX_L_G, buffer, sizeof(buffer));
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
    return ESP_OK;
}

