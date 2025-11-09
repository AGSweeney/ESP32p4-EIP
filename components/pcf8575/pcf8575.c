#include "pcf8575.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

bool pcf8575_init(pcf8575_t *dev, i2c_master_dev_handle_t i2c_dev)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;
    return true;
}

esp_err_t pcf8575_write(pcf8575_t *dev, uint16_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t payload[2] = {(uint8_t)(value & 0xFF), (uint8_t)(value >> 8)};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

esp_err_t pcf8575_read(pcf8575_t *dev, uint16_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t buffer[2] = {0};
    esp_err_t err = i2c_master_receive(dev->i2c_dev, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }
    *value = ((uint16_t)buffer[1] << 8) | buffer[0];
    return ESP_OK;
}

esp_err_t pcf8575_update_mask(pcf8575_t *dev, uint16_t mask, uint16_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t current = 0;
    esp_err_t err = pcf8575_read(dev, &current);
    if (err != ESP_OK)
    {
        return err;
    }
    current = (current & ~mask) | (value & mask);
    return pcf8575_write(dev, current);
}

esp_err_t pcf8575_write_pin(pcf8575_t *dev, uint8_t pin, bool level)
{
    if (pin > 15)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t mask = (uint16_t)1U << pin;
    return pcf8575_update_mask(dev, mask, level ? mask : 0);
}

esp_err_t pcf8575_read_pin(pcf8575_t *dev, uint8_t pin, bool *level)
{
    if (pin > 15 || !level)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t value = 0;
    esp_err_t err = pcf8575_read(dev, &value);
    if (err != ESP_OK)
    {
        return err;
    }
    *level = (value >> pin) & 0x1;
    return ESP_OK;
}

