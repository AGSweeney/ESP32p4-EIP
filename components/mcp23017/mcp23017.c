#include "mcp23017.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

static esp_err_t write_reg(mcp23017_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t payload[2] = {reg, value};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

static esp_err_t read_reg(mcp23017_t *dev, uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

bool mcp23017_init(mcp23017_t *dev,
                   i2c_master_dev_handle_t i2c_dev,
                   const mcp23017_config_t *cfg)
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
    if (write_reg(dev, MCP23017_REG_IODIRA, cfg->iodir_a) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_IODIRB, cfg->iodir_b) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_IPOLA, cfg->ipol_a) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_IPOLB, cfg->ipol_b) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_GPINTENA, cfg->gpinten_a) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_GPINTENB, cfg->gpinten_b) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_DEFVALA, cfg->defval_a) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_DEFVALB, cfg->defval_b) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_INTCONA, cfg->intcon_a) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_INTCONB, cfg->intcon_b) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_IOCON, cfg->iocon) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_GPPUA, cfg->gppu_a) != ESP_OK)
    {
        return false;
    }
    if (write_reg(dev, MCP23017_REG_GPPUB, cfg->gppu_b) != ESP_OK)
    {
        return false;
    }
    return true;
}

esp_err_t mcp23017_write_register(mcp23017_t *dev, uint8_t reg, uint8_t value)
{
    return write_reg(dev, reg, value);
}

esp_err_t mcp23017_read_register(mcp23017_t *dev, uint8_t reg, uint8_t *value)
{
    return read_reg(dev, reg, value);
}

esp_err_t mcp23017_write_gpio(mcp23017_t *dev, uint16_t value)
{
    uint8_t high = (uint8_t)(value >> 8);
    uint8_t low = (uint8_t)(value & 0xFF);
    if (write_reg(dev, MCP23017_REG_GPIOA, low) != ESP_OK)
    {
        return ESP_FAIL;
    }
    return write_reg(dev, MCP23017_REG_GPIOB, high);
}

esp_err_t mcp23017_read_gpio(mcp23017_t *dev, uint16_t *value)
{
    uint8_t low = 0;
    uint8_t high = 0;
    if (read_reg(dev, MCP23017_REG_GPIOA, &low) != ESP_OK)
    {
        return ESP_FAIL;
    }
    if (read_reg(dev, MCP23017_REG_GPIOB, &high) != ESP_OK)
    {
        return ESP_FAIL;
    }
    *value = ((uint16_t)high << 8) | low;
    return ESP_OK;
}

