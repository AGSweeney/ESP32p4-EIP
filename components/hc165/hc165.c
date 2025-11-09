#include "hc165.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"

static void pulse_clock(const hc165_t *dev)
{
    gpio_set_level(dev->config.gpio_clock, dev->config.clock_idle_high ? 0 : 1);
    gpio_set_level(dev->config.gpio_clock, dev->config.clock_idle_high ? 1 : 0);
}

static void latch_inputs(const hc165_t *dev)
{
    gpio_set_level(dev->config.gpio_latch, 0);
    esp_rom_delay_us(1);
    gpio_set_level(dev->config.gpio_latch, 1);
}

esp_err_t hc165_init(hc165_t *dev, const hc165_config_t *cfg)
{
    if (!dev || !cfg)
    {
        return ESP_ERR_INVALID_ARG;
    }
    dev->config = *cfg;

    gpio_config_t out_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << cfg->gpio_clock) |
                        (1ULL << cfg->gpio_latch),
    };
    esp_err_t err = gpio_config(&out_conf);
    if (err != ESP_OK)
    {
        return err;
    }

    gpio_config_t in_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << cfg->gpio_data,
        .pull_up_en = true,
    };
    err = gpio_config(&in_conf);
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

    gpio_set_level(cfg->gpio_clock, cfg->clock_idle_high ? 1 : 0);
    gpio_set_level(cfg->gpio_latch, 1);
    return ESP_OK;
}

esp_err_t hc165_shift_byte(hc165_t *dev, uint8_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    latch_inputs(dev);

    uint8_t result = 0;
    for (int i = 7; i >= 0; --i)
    {
        if (dev->config.sample_on_falling_edge)
        {
            pulse_clock(dev);
            result |= gpio_get_level(dev->config.gpio_data) << i;
        }
        else
        {
            result |= gpio_get_level(dev->config.gpio_data) << i;
            pulse_clock(dev);
        }
    }
    *value = result;
    return ESP_OK;
}

esp_err_t hc165_shift_buffer(hc165_t *dev, uint8_t *data, size_t length)
{
    if (!dev || (!data && length > 0))
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < length; ++i)
    {
        esp_err_t err = hc165_shift_byte(dev, &data[i]);
        if (err != ESP_OK)
        {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t hc165_set_output_enable(hc165_t *dev, bool enable)
{
    if (!dev || dev->config.gpio_oe < 0)
    {
        return ESP_ERR_INVALID_STATE;
    }
    gpio_set_level(dev->config.gpio_oe, enable ? 0 : 1);
    return ESP_OK;
}

