#include "mcp23008.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

static esp_err_t write_reg(mcp23008_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

static esp_err_t read_reg(mcp23008_t *dev, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

bool mcp23008_init(mcp23008_t *dev,
                   i2c_master_dev_handle_t i2c_dev,
                   const mcp23008_config_t *cfg)
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
    if (write_reg(dev, MCP23008_REG_IODIR, cfg->iodir) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23008_REG_IPOL, cfg->ipol) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23008_REG_GPINTEN, cfg->gpinten) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23008_REG_DEFVAL, cfg->defval) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23008_REG_INTCON, cfg->intcon) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23008_REG_IOCON, cfg->iocon) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23008_REG_GPPU, cfg->gppu) != ESP_OK)
    {
        return false;
    }
    return true;
}

esp_err_t mcp23008_write_register(mcp23008_t *dev, uint8_t reg, uint8_t value)
{
    return write_reg(dev, reg, value);
}

esp_err_t mcp23008_read_register(mcp23008_t *dev, uint8_t reg, uint8_t *value)
{
    return read_reg(dev, reg, value);
}

esp_err_t mcp23008_write_gpio(mcp23008_t *dev, uint8_t value)
{
    return write_reg(dev, MCP23008_REG_GPIO, value);
}

esp_err_t mcp23008_read_gpio(mcp23008_t *dev, uint8_t *value)
{
    return read_reg(dev, MCP23008_REG_GPIO, value);
}

static esp_err_t update_reg(mcp23008_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
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

esp_err_t mcp23008_set_direction(mcp23008_t *dev, uint8_t mask)
{
    return write_reg(dev, MCP23008_REG_IODIR, mask);
}

esp_err_t mcp23008_set_polarity(mcp23008_t *dev, uint8_t mask)
{
    return write_reg(dev, MCP23008_REG_IPOL, mask);
}

esp_err_t mcp23008_set_pullups(mcp23008_t *dev, uint8_t mask)
{
    return write_reg(dev, MCP23008_REG_GPPU, mask);
}

esp_err_t mcp23008_update_gpio_mask(mcp23008_t *dev, uint8_t mask, uint8_t value)
{
    return update_reg(dev, MCP23008_REG_OLAT, mask, value);
}

esp_err_t mcp23008_write_pin(mcp23008_t *dev, uint8_t pin, bool level)
{
    if (pin > 7)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t mask = 1U << pin;
    return mcp23008_update_gpio_mask(dev, mask, level ? mask : 0);
}

esp_err_t mcp23008_read_pin(mcp23008_t *dev, uint8_t pin, bool *level)
{
    if (pin > 7 || !level)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t value = 0;
    esp_err_t err = mcp23008_read_gpio(dev, &value);
    if (err != ESP_OK)
    {
        return err;
    }
    *level = (value >> pin) & 0x1;
    return ESP_OK;
}

