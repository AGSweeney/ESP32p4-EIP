# MCP3008 ESP-IDF Component

Basic SPI helper for the 8-channel, 10-bit MCP3008 ADC. Exposes an initialisation routine and a raw conversion helper.

## Directory Layout

```
components/mcp3008/
├── CMakeLists.txt
├── include/
│   └── mcp3008.h
└── mcp3008.c
```

## Public API

```c
esp_err_t mcp3008_init(mcp3008_t *dev,
                       spi_device_handle_t spi_dev,
                       int gpio_cs);

esp_err_t mcp3008_read_raw(mcp3008_t *dev, uint8_t channel,
                           bool single_ended, uint16_t *value);
esp_err_t mcp3008_read_average(mcp3008_t *dev, uint8_t channel,
                               bool single_ended, uint8_t samples, uint16_t *value);
esp_err_t mcp3008_read_voltage(mcp3008_t *dev, uint8_t channel,
                               bool single_ended, float reference_voltage, float *voltage);
```

- `gpio_cs` allows use of a manual chip-select pin. Pass `-1` to rely on the SPI device’s CS.
- `single_ended = true` selects single-ended mode; `false` selects differential (CH/CH+1 pairs).
- Returned `value` is 10-bit (0–1023). `mcp3008_read_average` repeats the conversion `samples` times and returns the integer average.
- `mcp3008_read_voltage` converts the raw result into volts using the reference voltage you provide.

## Example

```c
spi_device_handle_t spi_dev = /* created via spi_bus_add_device */;
mcp3008_t adc;
ESP_ERROR_CHECK(mcp3008_init(&adc, spi_dev, -1));

uint16_t reading = 0;
mcp3008_read_average(&adc, 0, true, 8, &reading);

float volts = 0.0f;
mcp3008_read_voltage(&adc, 0, true, 3.3f, &volts);
ESP_LOGI("MCP3008", "Channel0 = %u (~%.3f V)", reading, volts);
```

Tune the SPI clock speed and mode according to your hardware requirements (MCP3008 works with SPI mode 0).

