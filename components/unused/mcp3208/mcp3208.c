#include "mcp3208.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

esp_err_t mcp3208_init(mcp3208_t *dev,
                       spi_device_handle_t spi_dev,
                       int gpio_cs)
{
    if (!dev || !spi_dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    dev->spi_dev = spi_dev;
    dev->gpio_cs = gpio_cs;

    if (gpio_cs >= 0)
    {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << gpio_cs,
        };
        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK)
        {
            return err;
        }
        gpio_set_level(gpio_cs, 1);
    }
    return ESP_OK;
}

static inline void cs_select(const mcp3208_t *dev)
{
    if (dev->gpio_cs >= 0)
    {
        gpio_set_level(dev->gpio_cs, 0);
    }
}

static inline void cs_deselect(const mcp3208_t *dev)
{
    if (dev->gpio_cs >= 0)
    {
        gpio_set_level(dev->gpio_cs, 1);
    }
}

esp_err_t mcp3208_read_raw(mcp3208_t *dev, uint8_t channel, bool single_ended, uint16_t *value)
{
    if (!dev || !value || channel > 7)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t control = (single_ended ? 0x80 : 0x00) | ((channel & 0x07) << 4);
    uint8_t tx_data[3] = {0x06 | (control >> 7), (uint8_t)(control << 1), 0x00};
    uint8_t rx_data[3] = {0};

    spi_transaction_t t = {
        .length = 24,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    cs_select(dev);
    esp_err_t err = spi_device_transmit(dev->spi_dev, &t);
    cs_deselect(dev);

    if (err != ESP_OK)
    {
        return err;
    }

    *value = ((rx_data[1] & 0x0F) << 8) | rx_data[2];
    return ESP_OK;
}

esp_err_t mcp3208_read_average(mcp3208_t *dev, uint8_t channel, bool single_ended, uint8_t samples, uint16_t *value)
{
    if (!dev || !value || samples == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint32_t total = 0;
    for (uint8_t i = 0; i < samples; ++i)
    {
        uint16_t sample = 0;
        esp_err_t err = mcp3208_read_raw(dev, channel, single_ended, &sample);
        if (err != ESP_OK)
        {
            return err;
        }
        total += sample;
    }
    *value = (uint16_t)(total / samples);
    return ESP_OK;
}

esp_err_t mcp3208_read_voltage(mcp3208_t *dev, uint8_t channel, bool single_ended, float reference_voltage, float *voltage)
{
    if (!dev || !voltage || reference_voltage <= 0.0f)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t raw = 0;
    esp_err_t err = mcp3208_read_raw(dev, channel, single_ended, &raw);
    if (err != ESP_OK)
    {
        return err;
    }
    *voltage = (reference_voltage * (float)raw) / 4095.0f;
    return ESP_OK;
}

