# PCF8575 ESP-IDF Component

Simple wrappers for the NXP PCF8575 16-bit I²C I/O expander. Pins are quasi-bidirectional like the PCF8574; drive low for outputs, release high to read inputs.

## Directory Layout

```
components/pcf8575/
├── CMakeLists.txt
├── include/
│   └── pcf8575.h
└── pcf8575.c
```

## Public API

```c
bool pcf8575_init(pcf8575_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t pcf8575_write(pcf8575_t *dev, uint16_t value);
esp_err_t pcf8575_read(pcf8575_t *dev, uint16_t *value);
esp_err_t pcf8575_update_mask(pcf8575_t *dev, uint16_t mask, uint16_t value);
esp_err_t pcf8575_write_pin(pcf8575_t *dev, uint8_t pin, bool level);
esp_err_t pcf8575_read_pin(pcf8575_t *dev, uint8_t pin, bool *level);
```

## Example

```c
pcf8575_t io16;
pcf8575_init(&io16, dev_handle);

pcf8575_update_mask(&io16, 0x000F, 0x0003);  // pull P0/P1 low, release P2/P3

uint16_t state;
pcf8575_read(&io16, &state);
ESP_LOGI("PCF8575", "State: 0x%04X", state);
```

Chain devices by altering A0–A2 address pins. Handle the INT output in your application if you rely on change interrupts.

