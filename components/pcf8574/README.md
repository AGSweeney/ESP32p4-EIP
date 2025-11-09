# PCF8574 ESP-IDF Component

Lightweight scaffold for the NXP PCF8574 8-bit I²C I/O expander. The device provides quasi-bidirectional pins; here we expose basic read/write helpers and leave pin direction handling to the application (drive low for outputs, release high for inputs).

## Directory Layout

```
components/pcf8574/
├── CMakeLists.txt
├── include/
│   └── pcf8574.h
└── pcf8574.c
```

## Public API

```c
bool pcf8574_init(pcf8574_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t pcf8574_write(pcf8574_t *dev, uint8_t value);
esp_err_t pcf8574_read(pcf8574_t *dev, uint8_t *value);
esp_err_t pcf8574_update_mask(pcf8574_t *dev, uint8_t mask, uint8_t value);
esp_err_t pcf8574_write_pin(pcf8574_t *dev, uint8_t pin, bool level);
esp_err_t pcf8574_read_pin(pcf8574_t *dev, uint8_t pin, bool *level);
```

- Call `pcf8574_init` with an ESP-IDF I²C device handle.
- Use `pcf8574_write` or `pcf8574_update_mask` to drive outputs (bits set to 0 pull low, bits set to 1 release high).
- The single-pin helpers simplify toggling or sampling individual bits without disturbing the remaining pins.

## Quick Example

```c
pcf8574_t io;
i2c_master_dev_handle_t dev_handle = /* created earlier */;
pcf8574_init(&io, dev_handle);

pcf8574_update_mask(&io, 0x0F, 0x01);  // set P0 low, release P1..P3

uint8_t state = 0;
pcf8574_read(&io, &state);
ESP_LOGI("PCF8574", "Pins: 0x%02X", state);
```

The PCF8574’s address is set using A0–A2 (default `0x20`). Add your own interrupt handler for the INT line if required.

