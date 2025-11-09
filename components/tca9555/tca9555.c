#include "tca9555.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

static esp_err_t write_reg(tca9555_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

static esp_err_t read_reg(tca9555_t *dev, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

bool tca9555_init(tca9555_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const tca9555_config_t *cfg)
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

    if (write_reg(dev, TCA9555_REG_CONFIG0, cfg->config0) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, TCA9555_REG_CONFIG1, cfg->config1) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, TCA9555_REG_POL0, cfg->polarity0) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, TCA9555_REG_POL1, cfg->polarity1) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, TCA9555_REG_OUTPUT0, cfg->output0) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, TCA9555_REG_OUTPUT1, cfg->output1) != ESP_OK)
    {
        return false;
    }
    return true;
}

esp_err_t tca9555_write_register(tca9555_t *dev, uint8_t reg, uint8_t value)
{
    return write_reg(dev, reg, value);
}

esp_err_t tca9555_read_register(tca9555_t *dev, uint8_t reg, uint8_t *value)
{
    return read_reg(dev, reg, value);
}

esp_err_t tca9555_write_gpio(tca9555_t *dev, uint16_t value)
{
    if (write_reg(dev, TCA9555_REG_OUTPUT0, (uint8_t)(value & 0xFF)) != ESP_OK)
    {
        return ESP_FAIL;
    }
    return write_reg(dev, TCA9555_REG_OUTPUT1, (uint8_t)(value >> 8));
}

esp_err_t tca9555_read_gpio(tca9555_t *dev, uint16_t *value)
{
    uint8_t low = 0;
    uint8_t high = 0;
    if (read_reg(dev, TCA9555_REG_INPUT0, &low) != ESP_OK)
    {
        return ESP_FAIL;
    }
    if (read_reg(dev, TCA9555_REG_INPUT1, &high) != ESP_OK)
    {
        return ESP_FAIL;
    }
    *value = ((uint16_t)high << 8) | low;
    return ESP_OK;
}

static esp_err_t write_pair(tca9555_t *dev, uint8_t reg, uint16_t value)
{
    esp_err_t err = write_reg(dev, reg, (uint8_t)(value & 0xFF));
    if (err != ESP_OK)
    {
        return err;
    }
    return write_reg(dev, reg + 1, (uint8_t)(value >> 8));
}

static esp_err_t read_pair(tca9555_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t low = 0;
    uint8_t high = 0;
    esp_err_t err = read_reg(dev, reg, &low);
    if (err != ESP_OK)
    {
        return err;
    }
    err = read_reg(dev, reg + 1, &high);
    if (err != ESP_OK)
    {
        return err;
    }
    *value = ((uint16_t)high << 8) | low;
    return ESP_OK;
}

static esp_err_t update_pair(tca9555_t *dev, uint8_t reg, uint16_t mask, uint16_t value)
{
    uint16_t current = 0;
    esp_err_t err = read_pair(dev, reg, &current);
    if (err != ESP_OK)
    {
        return err;
    }
    current = (current & ~mask) | (value & mask);
    return write_pair(dev, reg, current);
}

esp_err_t tca9555_set_direction(tca9555_t *dev, uint16_t mask)
{
    return write_pair(dev, TCA9555_REG_CONFIG0, mask);
}

esp_err_t tca9555_set_polarity(tca9555_t *dev, uint16_t mask)
{
    return write_pair(dev, TCA9555_REG_POL0, mask);
}

esp_err_t tca9555_update_gpio_mask(tca9555_t *dev, uint16_t mask, uint16_t value)
{
    return update_pair(dev, TCA9555_REG_OUTPUT0, mask, value);
}

esp_err_t tca9555_write_pin(tca9555_t *dev, uint8_t pin, bool level)
{
    if (pin > 15)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t mask = (uint16_t)1U << pin;
    return tca9555_update_gpio_mask(dev, mask, level ? mask : 0);
}

esp_err_t tca9555_read_pin(tca9555_t *dev, uint8_t pin, bool *level)
{
    if (pin > 15 || !level)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t value = 0;
    esp_err_t err = tca9555_read_gpio(dev, &value);
    if (err != ESP_OK)
    {
        return err;
    }
    *level = (value >> pin) & 0x1;
    return ESP_OK;
}

