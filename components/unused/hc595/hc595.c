#include "hc595.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"

static void pulse_clock(const hc595_t *dev)
{
    gpio_set_level(dev->config.gpio_clock, dev->config.clock_idle_high ? 0 : 1);
    gpio_set_level(dev->config.gpio_clock, dev->config.clock_idle_high ? 1 : 0);
}

static void latch_outputs(const hc595_t *dev)
{
    gpio_set_level(dev->config.gpio_latch, 1);
    gpio_set_level(dev->config.gpio_latch, 0);
}

esp_err_t hc595_init(hc595_t *dev, const hc595_config_t *cfg)
{
    if (!dev || !cfg)
    {
        return ESP_ERR_INVALID_ARG;
    }
    dev->config = *cfg;

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << cfg->gpio_data) |
                        (1ULL << cfg->gpio_clock) |
                        (1ULL << cfg->gpio_latch),
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        return err;
    }

    if (cfg->gpio_oe >= 0)
    {
        gpio_config_t oe_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << cfg->gpio_oe,
        };
        err = gpio_config(&oe_conf);
        if (err != ESP_OK)
        {
            return err;
        }
        gpio_set_level(cfg->gpio_oe, 0);
    }

    if (cfg->gpio_clear >= 0)
    {
        gpio_config_t clr_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << cfg->gpio_clear,
        };
        err = gpio_config(&clr_conf);
        if (err != ESP_OK)
        {
            return err;
        }
        gpio_set_level(cfg->gpio_clear, 1);
    }

    gpio_set_level(cfg->gpio_data, 0);
    gpio_set_level(cfg->gpio_clock, cfg->clock_idle_high ? 1 : 0);
    gpio_set_level(cfg->gpio_latch, 0);

    return ESP_OK;
}

esp_err_t hc595_shift_byte(hc595_t *dev, uint8_t value)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = 7; i >= 0; --i)
    {
        gpio_set_level(dev->config.gpio_data, (value >> i) & 0x1);
        pulse_clock(dev);
    }
    latch_outputs(dev);
    return ESP_OK;
}

esp_err_t hc595_shift_buffer(hc595_t *dev, const uint8_t *data, size_t length)
{
    if (!dev || (!data && length > 0))
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < length; ++i)
    {
        esp_err_t err = hc595_shift_byte(dev, data[i]);
        if (err != ESP_OK)
        {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t hc595_set_output_enable(hc595_t *dev, bool enable)
{
    if (!dev || dev->config.gpio_oe < 0)
    {
        return ESP_ERR_INVALID_STATE;
    }
    gpio_set_level(dev->config.gpio_oe, enable ? 0 : 1);
    return ESP_OK;
}

esp_err_t hc595_clear(hc595_t *dev)
{
    if (!dev || dev->config.gpio_clear < 0)
    {
        return ESP_ERR_INVALID_STATE;
    }
    gpio_set_level(dev->config.gpio_clear, 0);
    esp_rom_delay_us(1);
    gpio_set_level(dev->config.gpio_clear, 1);
    return ESP_OK;
}

