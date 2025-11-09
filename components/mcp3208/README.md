# MCP3208 ESP-IDF Component

Helper routines for the Microchip MCP3208 8-channel, 12-bit SPI ADC.

## Directory Layout

```
components/mcp3208/
├── CMakeLists.txt
├── include/
│   └── mcp3208.h
└── mcp3208.c
```

## Public API

```c
esp_err_t mcp3208_init(mcp3208_t *dev,
                       spi_device_handle_t spi_dev,
                       int gpio_cs);

esp_err_t mcp3208_read_raw(mcp3208_t *dev, uint8_t channel,
                           bool single_ended, uint16_t *value);
esp_err_t mcp3208_read_average(mcp3208_t *dev, uint8_t channel,
                               bool single_ended, uint8_t samples, uint16_t *value);
esp_err_t mcp3208_read_voltage(mcp3208_t *dev, uint8_t channel,
                               bool single_ended, float reference_voltage, float *voltage);
```

- Works similarly to the MCP3008 helper but returns 12-bit samples.
- `single_ended = true` selects single-ended mode; `false` selects differential inputs.
- `gpio_cs` allows you to manage CS manually; pass `-1` to rely on the SPI driver.

## Example

```c
spi_device_handle_t spi_dev = /* configured for SPI mode 0 */;
mcp3208_t adc;
mcp3208_init(&adc, spi_dev, -1);

uint16_t raw;
mcp3208_read_average(&adc, 3, true, 16, &raw);

float volts = 0.0f;
mcp3208_read_voltage(&adc, 3, true, 3.3f, &volts);
ESP_LOGI("MCP3208", "Channel3 = %u (~%.3f V)", raw, volts);
```

Scale the raw values (0–4095) to voltage/current as needed for your application.

