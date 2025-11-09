#include "tca9534.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

static esp_err_t write_reg(tca9534_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

static esp_err_t read_reg(tca9534_t *dev, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

bool tca9534_init(tca9534_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const tca9534_config_t *cfg)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;

    if (!cfg)
    {
        return true;
    }

    if (write_reg(dev, TCA9534_REG_CONFIG, cfg->direction) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, TCA9534_REG_POLARITY, cfg->polarity) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, TCA9534_REG_OUTPUT, cfg->output) != ESP_OK)
    {
        return false;
    }
    return true;
}

esp_err_t tca9534_write_register(tca9534_t *dev, uint8_t reg, uint8_t value)
{
    return write_reg(dev, reg, value);
}

esp_err_t tca9534_read_register(tca9534_t *dev, uint8_t reg, uint8_t *value)
{
    return read_reg(dev, reg, value);
}

esp_err_t tca9534_write_gpio(tca9534_t *dev, uint8_t value)
{
    return write_reg(dev, TCA9534_REG_OUTPUT, value);
}

esp_err_t tca9534_read_gpio(tca9534_t *dev, uint8_t *value)
{
    return read_reg(dev, TCA9534_REG_INPUT, value);
}

static esp_err_t update_reg(tca9534_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current = 0;
    esp_err_t err = read_reg(dev, reg, &current);
    if (err != ESP_OK)
    {
        return err;
    }
    current = (current & ~mask) | (value & mask);
    return write_reg(dev, reg, current);
}

esp_err_t tca9534_set_direction(tca9534_t *dev, uint8_t mask)
{
    return write_reg(dev, TCA9534_REG_CONFIG, mask);
}

esp_err_t tca9534_set_polarity(tca9534_t *dev, uint8_t mask)
{
    return write_reg(dev, TCA9534_REG_POLARITY, mask);
}

esp_err_t tca9534_update_gpio_mask(tca9534_t *dev, uint8_t mask, uint8_t value)
{
    return update_reg(dev, TCA9534_REG_OUTPUT, mask, value);
}

esp_err_t tca9534_write_pin(tca9534_t *dev, uint8_t pin, bool level)
{
    if (pin > 7)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t mask = (uint8_t)1U << pin;
    return tca9534_update_gpio_mask(dev, mask, level ? mask : 0);
}

esp_err_t tca9534_read_pin(tca9534_t *dev, uint8_t pin, bool *level)
{
    if (pin > 7 || !level)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t value = 0;
    esp_err_t err = tca9534_read_gpio(dev, &value);
    if (err != ESP_OK)
    {
        return err;
    }
    *level = (value >> pin) & 0x1;
    return ESP_OK;
}

