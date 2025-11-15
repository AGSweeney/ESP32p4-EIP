#include "mcp4725.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

bool mcp4725_init(mcp4725_t *dev, i2c_master_dev_handle_t i2c_dev)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;
    return true;
}

static esp_err_t write_command(mcp4725_t *dev, uint8_t cmd, uint16_t value)
{
    uint8_t payload[3];
    payload[0] = cmd;
    payload[1] = (uint8_t)(value >> 4);
    payload[2] = (uint8_t)((value & 0x0F) << 4);
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

esp_err_t mcp4725_write_dac(mcp4725_t *dev, uint16_t value)
{
    return mcp4725_write_dac_mode(dev, value, MCP4725_POWER_MODE_NORMAL);
}

esp_err_t mcp4725_write_dac_mode(mcp4725_t *dev, uint16_t value, mcp4725_power_mode_t mode)
{
    if (!dev || value > 0x0FFF || mode > MCP4725_POWER_MODE_PD_500K)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t payload[2] = {
        (uint8_t)(((mode & 0x03) << 4) | ((value >> 8) & 0x0F)),
        (uint8_t)(value & 0xFF)};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

esp_err_t mcp4725_write_eeprom(mcp4725_t *dev, uint16_t value)
{
    if (!dev || value > 0x0FFF)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return write_command(dev, 0x60, value);
}

esp_err_t mcp4725_set_power_mode(mcp4725_t *dev, mcp4725_power_mode_t mode)
{
    if (!dev || mode > MCP4725_POWER_MODE_PD_500K)
    {
        return ESP_ERR_INVALID_ARG;
    }
    mcp4725_status_t status = {0};
    esp_err_t err = mcp4725_read_status(dev, &status);
    if (err != ESP_OK)
    {
        return err;
    }
    return mcp4725_write_dac_mode(dev, status.dac_value, mode);
}

esp_err_t mcp4725_read_status(mcp4725_t *dev, mcp4725_status_t *status)
{
    if (!dev || !status)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t buffer[5] = {0};
    esp_err_t err = i2c_master_receive(dev->i2c_dev, buffer, sizeof(buffer), pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }
    status->busy = (buffer[0] & 0x80) != 0;
    status->power_mode = (mcp4725_power_mode_t)((buffer[0] >> 1) & 0x03);
    status->dac_value = (uint16_t)(((buffer[0] & 0x0F) << 8) | buffer[1]);
    uint16_t eeprom_raw = ((uint16_t)buffer[3] << 8) | buffer[4];
    status->eeprom_value = (uint16_t)(eeprom_raw >> 4);
    return ESP_OK;
}

