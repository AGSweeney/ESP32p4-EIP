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

