# TCA9555 ESP-IDF Component

Minimal driver scaffold for the TI TCA9555 16-bit I²C GPIO expander. This mirrors the structure of the MCP23017/TCA9555 family and exposes simple helpers to configure direction, polarity, and read/write the GPIO latches.

## Directory Layout

```
components/tca9555/
├── CMakeLists.txt
├── include/
│   └── tca9555.h
└── tca9555.c
```

## Public API

```c
typedef struct {
    uint8_t config0;   // 1 = input, 0 = output
    uint8_t config1;
    uint8_t polarity0; // 1 = invert
    uint8_t polarity1;
    uint8_t output0;   // initial latch state
    uint8_t output1;
} tca9555_config_t;

bool tca9555_init(tca9555_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const tca9555_config_t *cfg);

esp_err_t tca9555_write_gpio(tca9555_t *dev, uint16_t value);
esp_err_t tca9555_read_gpio(tca9555_t *dev, uint16_t *value);
esp_err_t tca9555_write_register(tca9555_t *dev, uint8_t reg, uint8_t value);
esp_err_t tca9555_read_register(tca9555_t *dev, uint8_t reg, uint8_t *value);
esp_err_t tca9555_set_direction(tca9555_t *dev, uint16_t mask);
esp_err_t tca9555_set_polarity(tca9555_t *dev, uint16_t mask);
esp_err_t tca9555_update_gpio_mask(tca9555_t *dev, uint16_t mask, uint16_t value);
esp_err_t tca9555_write_pin(tca9555_t *dev, uint8_t pin, bool level);
esp_err_t tca9555_read_pin(tca9555_t *dev, uint8_t pin, bool *level);
```

## Example

```c
tca9555_t expander;
tca9555_config_t cfg = {
    .config0 = 0x0F,   // lower nibble outputs, upper nibble inputs
    .config1 = 0xFF,   // port 1 inputs
    .polarity0 = 0x00,
    .polarity1 = 0x00,
    .output0 = 0x05,
    .output1 = 0x00,
};

i2c_master_dev_handle_t dev_handle = /* created previously */;
if (!tca9555_init(&expander, dev_handle, &cfg)) {
    ESP_LOGE("TCA9555", "Init failed");
}

tca9555_update_gpio_mask(&expander, 0x000F, 0x0003);
bool input;
tca9555_read_pin(&expander, 12, &input);
```

Set the address via A0–A2 pins (default `0x20`). The helper only handles BANK=0 register layout (default). For interrupt use, configure the INT pin manually in your application code.

