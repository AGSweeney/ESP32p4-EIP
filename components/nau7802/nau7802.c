#include "nau7802.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

bool nau7802_init(nau7802_t *dev, i2c_master_dev_handle_t i2c_dev)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;
    return true;
}

esp_err_t nau7802_write_register(nau7802_t *dev, uint8_t reg, uint8_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

esp_err_t nau7802_read_register(nau7802_t *dev, uint8_t reg, uint8_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

esp_err_t nau7802_read_conversion(nau7802_t *dev, int32_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = 0x12; /* ADCO_B2 register */
    uint8_t buffer[3] = {0};
    esp_err_t err = i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }
    int32_t raw = ((int32_t)buffer[0] << 16) | ((int32_t)buffer[1] << 8) | buffer[2];
    if (raw & 0x800000)
    {
        raw |= 0xFF000000;
    }
    *value = raw;
    return ESP_OK;
}

static esp_err_t modify_register(nau7802_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current = 0;
    esp_err_t err = nau7802_read_register(dev, reg, &current);
    if (err != ESP_OK)
    {
        return err;
    }
    current = (current & ~mask) | (value & mask);
    return nau7802_write_register(dev, reg, current);
}

esp_err_t nau7802_soft_reset(nau7802_t *dev)
{
    esp_err_t err = modify_register(dev, NAU7802_REG_PU_CTRL, NAU7802_PU_CTRL_PUR, NAU7802_PU_CTRL_PUR);
    if (err != ESP_OK)
    {
        return err;
    }
    for (int i = 0; i < 50; ++i)
    {
        uint8_t reg = 0;
        err = nau7802_read_register(dev, NAU7802_REG_PU_CTRL, &reg);
        if (err != ESP_OK)
        {
            return err;
        }
        if ((reg & NAU7802_PU_CTRL_PUR) == 0)
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t nau7802_power_up(nau7802_t *dev)
{
    esp_err_t err = modify_register(dev, NAU7802_REG_PU_CTRL, NAU7802_PU_CTRL_PUD | NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL_PUD | NAU7802_PU_CTRL_PUA);
    if (err != ESP_OK)
    {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
    return ESP_OK;
}

esp_err_t nau7802_power_down(nau7802_t *dev)
{
    return modify_register(dev, NAU7802_REG_PU_CTRL, NAU7802_PU_CTRL_PUD | NAU7802_PU_CTRL_PUA, 0);
}

esp_err_t nau7802_set_gain(nau7802_t *dev, uint8_t gain)
{
    gain &= 0x07;
    return modify_register(dev, NAU7802_REG_CTRL1, 0x07, gain);
}

esp_err_t nau7802_set_sample_rate(nau7802_t *dev, uint8_t rate)
{
    rate &= 0x07;
    return modify_register(dev, NAU7802_REG_CTRL1, 0x70, (uint8_t)(rate << 4));
}

esp_err_t nau7802_calibrate(nau7802_t *dev, bool internal_reference)
{
    uint8_t mode = internal_reference ? NAU7802_CTRL2_CALMOD_INTERNAL : NAU7802_CTRL2_CALMOD_EXTERNAL;
    esp_err_t err = modify_register(dev, NAU7802_REG_CTRL2, NAU7802_CTRL2_CALMOD_INTERNAL | NAU7802_CTRL2_CALMOD_EXTERNAL, mode);
    if (err != ESP_OK)
    {
        return err;
    }
    err = modify_register(dev, NAU7802_REG_CTRL2, NAU7802_CTRL2_CAL_START, NAU7802_CTRL2_CAL_START);
    if (err != ESP_OK)
    {
        return err;
    }
    for (int i = 0; i < 100; ++i)
    {
        uint8_t pu = 0;
        err = nau7802_read_register(dev, NAU7802_REG_PU_CTRL, &pu);
        if (err != ESP_OK)
        {
            return err;
        }
        if (pu & NAU7802_PU_CTRL_CRRDY)
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t nau7802_is_data_ready(nau7802_t *dev, bool *ready)
{
    if (!ready)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = 0;
    esp_err_t err = nau7802_read_register(dev, NAU7802_REG_PU_CTRL, &reg);
    if (err != ESP_OK)
    {
        return err;
    }
    *ready = (reg & NAU7802_PU_CTRL_DRDY) != 0;
    return ESP_OK;
}

esp_err_t nau7802_wait_ready(nau7802_t *dev, uint32_t timeout_ms)
{
    uint32_t waited = 0;
    const uint32_t step_ms = 5;
    while (waited <= timeout_ms)
    {
        bool ready = false;
        esp_err_t err = nau7802_is_data_ready(dev, &ready);
        if (err != ESP_OK)
        {
            return err;
        }
        if (ready)
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        waited += step_ms;
    }
    return ESP_ERR_TIMEOUT;
}

