#include "hx711.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void pulse_clock(const hx711_t *dev)
{
    gpio_set_level(dev->config.gpio_sck, 1);
    esp_rom_delay_us(1);
    gpio_set_level(dev->config.gpio_sck, 0);
    esp_rom_delay_us(1);
}

esp_err_t hx711_init(hx711_t *dev, const hx711_config_t *cfg)
{
    if (!dev || !cfg)
    {
        return ESP_ERR_INVALID_ARG;
    }
    dev->config = *cfg;

    gpio_config_t dout_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << cfg->gpio_dout,
        .pull_up_en = true,
    };
    esp_err_t err = gpio_config(&dout_conf);
    if (err != ESP_OK)
    {
        return err;
    }

    gpio_config_t sck_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << cfg->gpio_sck,
    };
    err = gpio_config(&sck_conf);
    if (err != ESP_OK)
    {
        return err;
    }

    gpio_set_level(cfg->gpio_sck, 0);
    return hx711_set_gain(dev, cfg->gain);
}

esp_err_t hx711_set_gain(hx711_t *dev, hx711_gain_t gain)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    dev->config.gain = gain;

    // Send gain-setting pulses (no data read)
    for (int i = 0; i < (int)gain; ++i)
    {
        pulse_clock(dev);
    }
    return ESP_OK;
}

esp_err_t hx711_read_raw(hx711_t *dev, int32_t *value)
{
    if (!dev || !value)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = hx711_wait_ready(dev, 200);
    if (err != ESP_OK)
    {
        return err;
    }

    int32_t raw = 0;
    for (int i = 0; i < 24; ++i)
    {
        pulse_clock(dev);
        raw = (raw << 1) | gpio_get_level(dev->config.gpio_dout);
    }

    // Pulse extra times to set gain for next reading
    for (int i = 0; i < (int)dev->config.gain; ++i)
    {
        pulse_clock(dev);
    }

    if (raw & 0x800000)
    {
        raw |= ~0xFFFFFF;
    }
    *value = raw;
    return ESP_OK;
}

bool hx711_is_ready(hx711_t *dev)
{
    if (!dev)
    {
        return false;
    }
    return gpio_get_level(dev->config.gpio_dout) == 0;
}

esp_err_t hx711_wait_ready(hx711_t *dev, uint32_t timeout_ms)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint32_t waited = 0;
    while (waited <= timeout_ms)
    {
        if (hx711_is_ready(dev))
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        waited += 1;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t hx711_read_average(hx711_t *dev, uint8_t samples, int32_t *value)
{
    if (!dev || !value || samples == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    int64_t total = 0;
    for (uint8_t i = 0; i < samples; ++i)
    {
        int32_t sample = 0;
        esp_err_t err = hx711_read_raw(dev, &sample);
        if (err != ESP_OK)
        {
            return err;
        }
        total += sample;
    }
    *value = (int32_t)(total / samples);
    return ESP_OK;
}

esp_err_t hx711_power_down(hx711_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_set_level(dev->config.gpio_sck, 1);
    esp_rom_delay_us(80);
    return ESP_OK;
}

esp_err_t hx711_power_up(hx711_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_set_level(dev->config.gpio_sck, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    return ESP_OK;
}

