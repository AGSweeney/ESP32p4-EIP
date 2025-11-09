#include "pcf8574.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

bool pcf8574_init(pcf8574_t *dev, i2c_master_dev_handle_t i2c_dev)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;
    return true;
}

esp_err_t pcf8574_write(pcf8574_t *dev, uint8_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit(dev->i2c_dev, &value, 1, pdMS_TO_TICKS(100));
}

esp_err_t pcf8574_read(pcf8574_t *dev, uint8_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_receive(dev->i2c_dev, value, 1, pdMS_TO_TICKS(100));
}

esp_err_t pcf8574_update_mask(pcf8574_t *dev, uint8_t mask, uint8_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t current = 0;
    esp_err_t err = pcf8574_read(dev, &current);
    if (err != ESP_OK)
    {
        return err;
    }
    current = (current & ~mask) | (value & mask);
    return pcf8574_write(dev, current);
}

esp_err_t pcf8574_write_pin(pcf8574_t *dev, uint8_t pin, bool level)
{
    if (pin > 7)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t mask = (uint8_t)1U << pin;
    return pcf8574_update_mask(dev, mask, level ? mask : 0);
}

esp_err_t pcf8574_read_pin(pcf8574_t *dev, uint8_t pin, bool *level)
{
    if (pin > 7 || !level)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t value = 0;
    esp_err_t err = pcf8574_read(dev, &value);
    if (err != ESP_OK)
    {
        return err;
    }
    *level = (value >> pin) & 0x1;
    return ESP_OK;
}

