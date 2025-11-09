# MP23008 ESP-IDF Component

Minimal support library for the Microchip/Adafruit MP23008 (MCP23008-compatible) I²C 8-bit I/O expander. This component provides register definitions and simple read/write helpers; it does not include any runtime polling or interrupt handling logic.

## Directory Layout

```
components/mp23008/
├── CMakeLists.txt
├── include/
│   └── mp23008.h
└── mp23008.c
```

## Public API

```c
typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} mp23008_t;

typedef struct {
    uint8_t iodir;    // 1 = input, 0 = output
    uint8_t ipol;     // 1 = inverted input
    uint8_t gpinten;  // interrupt-on-change enable
    uint8_t defval;   // default compare for interrupt
    uint8_t intcon;   // interrupt control (compare vs previous)
    uint8_t iocon;    // configuration bits
    uint8_t gppu;     // internal pull-ups
} mp23008_config_t;

bool mp23008_init(mp23008_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const mp23008_config_t *cfg);

esp_err_t mp23008_write_register(mp23008_t *dev, uint8_t reg, uint8_t value);
esp_err_t mp23008_read_register(mp23008_t *dev, uint8_t reg, uint8_t *value);

esp_err_t mp23008_write_gpio(mp23008_t *dev, uint8_t value);
esp_err_t mp23008_read_gpio(mp23008_t *dev, uint8_t *value);
```

- Call `mp23008_init` once you have created an ESP-IDF master device handle (`i2c_master_bus_add_device`). Pass a `mp23008_config_t` to initialise direction/pullups, or `NULL` to leave registers untouched.
- Use `mp23008_write_gpio`/`mp23008_read_gpio` to drive or sample the 8-bit port when the device is in output/input mode.
- `mp23008_write_register` / `mp23008_read_register` give you direct access to any register if you need more control.

## Quick Example

```c
mp23008_t expander;
mp23008_config_t cfg = {
    .iodir = 0xF0,    // upper 4 bits input, lower 4 bits output
    .ipol = 0x00,
    .gpinten = 0xF0,  // interrupt on upper inputs (optional)
    .defval = 0x00,
    .intcon = 0xF0,
    .iocon = 0x04,    // open-drain INT, sequential disabled
    .gppu = 0xF0,     // pull-ups on input bits
};

i2c_master_dev_handle_t io_handle = /* created via i2c_master_bus_add_device */;
if (!mp23008_init(&expander, io_handle, &cfg)) {
    // handle error
}

// Toggle lower nibble LEDs
mp23008_write_gpio(&expander, 0x0F);

uint8_t inputs;
mp23008_read_gpio(&expander, &inputs);
```

## Notes

- Default I²C address is `0x20`; wire A0-A2 accordingly if you need multiple expanders.
- All I²C operations use blocking `i2c_master_transmit` / `i2c_master_transmit_receive` with a default 100 ms timeout.
- Interrupt support is not provided here; enable it via `gpinten`, route the INT pin to a GPIO, and handle the interrupt in your application if required.
- If you need MCP23017 (16-bit) support, extend the register definitions and helper functions similarly but add bank-group handling.

