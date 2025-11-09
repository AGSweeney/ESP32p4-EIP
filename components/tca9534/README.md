# TCA9534 ESP-IDF Component

Small helper library for the Texas Instruments TCA9534(TCA9534DWR) 8-bit I²C I/O expander. The component exposes register definitions and blocking read/write helpers using ESP-IDF’s `driver/i2c_master.h` API. No background tasks or interrupt handling are included—wire it into your own runtime as needed.

## Directory Layout

```
components/tca9534/
├── CMakeLists.txt
├── include/
│   └── tca9534.h
└── tca9534.c
```

## Public API

```c
typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} tca9534_t;

typedef struct {
    uint8_t direction; /* 1 = input, 0 = output */
    uint8_t polarity;  /* 1 = inverted input */
    uint8_t output;    /* initial output state */
} tca9534_config_t;

bool tca9534_init(tca9534_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const tca9534_config_t *cfg);

esp_err_t tca9534_write_register(tca9534_t *dev, uint8_t reg, uint8_t value);
esp_err_t tca9534_read_register(tca9534_t *dev, uint8_t reg, uint8_t *value);

esp_err_t tca9534_write_gpio(tca9534_t *dev, uint8_t value);
esp_err_t tca9534_read_gpio(tca9534_t *dev, uint8_t *value);
```

- `tca9534_init`: Provide an ESP-IDF I²C device handle and optional config. When `cfg` is `NULL` no registers are touched.
- `tca9534_write_gpio` / `tca9534_read_gpio`: Set or sample the 8-bit port (use after configuring direction bits).
- The low-level register helpers let you fine-tune polarity, interrupts, etc., as required.

## Quick Example

```c
tca9534_t expander;
tca9534_config_t cfg = {
    .direction = 0xF0,  // P7..P4 inputs, P3..P0 outputs
    .polarity  = 0x00,  // normal polarity
    .output    = 0x00,  // outputs low initially
};

i2c_master_dev_handle_t dev_handle = /* created via i2c_master_bus_add_device */;

if (!tca9534_init(&expander, dev_handle, &cfg)) {
    ESP_LOGE("TCA9534", "Init failed");
    return;
}

// Drive lower nibble high (assuming direction configured as outputs)
tca9534_write_gpio(&expander, 0x0F);

uint8_t inputs = 0;
tca9534_read_gpio(&expander, &inputs);
ESP_LOGI("TCA9534", "Input state 0x%02X", inputs);
```

## Notes

- Default device address is `0x20`. The address can be changed by wiring pins A0–A2.
- Registers:
  - `INPUT` (`0x00`) – read-only input port.
  - `OUTPUT` (`0x01`) – write desired output state; read gives last value.
  - `POLARITY` (`0x02`) – invert individual inputs (1 = inverted).
  - `CONFIG` (`0x03`) – direction bits (1 = input, 0 = output).
- Interrupts are not configured here; hook `POLARITY`, `CONFIG`, and the INT output to your MCU if you need change notifications.
- The device is similar to the MCP23008; you can use both components on the same I²C bus.

