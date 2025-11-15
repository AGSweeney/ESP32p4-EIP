# 74HC595 ESP-IDF Component

Helper library for driving the classic 74HC595 serial-in, parallel-out shift register from ESP-IDF. It assumes you control the register via three GPIO lines (SER/DATA, SRCLK, RCLK) plus optional OE and SRCLR lines.

## Directory Layout

```
components/hc595/
├── CMakeLists.txt
├── include/
│   └── hc595.h
└── hc595.c
```

## Public API

```c
typedef struct {
    int gpio_data;
    int gpio_clock;
    int gpio_latch;
    int gpio_oe;      // optional, set -1 if unused
    int gpio_clear;   // optional, set -1 if unused
    bool clock_idle_high;
} hc595_config_t;

esp_err_t hc595_init(hc595_t *dev, const hc595_config_t *cfg);
esp_err_t hc595_shift_byte(hc595_t *dev, uint8_t value);
esp_err_t hc595_shift_buffer(hc595_t *dev, const uint8_t *data, size_t length);
esp_err_t hc595_set_output_enable(hc595_t *dev, bool enable);
esp_err_t hc595_clear(hc595_t *dev);
```

- `hc595_init` configures the specified GPIOs as outputs, sets initial levels, and stores the configuration for later shifts.
- `hc595_shift_byte` shifts one byte MSB-first into the register and toggles the latch line.
- `hc595_shift_buffer` lets you push multiple bytes, useful for cascading devices.
- `hc595_set_output_enable` drives OE low to enable outputs (inverted pin). If you don't use OE, leave `gpio_oe = -1`.
- `hc595_clear` pulses the SRCLR pin (active-low) to clear all outputs—leave `gpio_clear = -1` if unused.

## Quick Example

```c
hc595_config_t cfg = {
    .gpio_data = 4,
    .gpio_clock = 5,
    .gpio_latch = 6,
    .gpio_oe = -1,
    .gpio_clear = -1,
    .clock_idle_high = false,
};

hc595_t shiftreg;
ESP_ERROR_CHECK(hc595_init(&shiftreg, &cfg));

// Shift out 0b10101010
hc595_shift_byte(&shiftreg, 0xAA);

// Shift an array (cascaded registers)
uint8_t buffer[] = {0xAA, 0x55, 0xFF};
hc595_shift_buffer(&shiftreg, buffer, sizeof(buffer));
```

## Notes

- Adjust `clock_idle_high` if you need to clock data on the falling edge instead of the rising edge.
- After shifting data, the outputs only change when RCLK (latch) is pulsed—this library toggles latch after every shift. If you need finer control, expose a manual latch function.
- To cascade multiple 74HC595s, connect QH' of one chip to the SER input of the next and call `hc595_shift_buffer`.
- Outputs are latched; if OE is high (disabled), pins float. Use `hc595_set_output_enable` to control power-saving or to tri-state the outputs.

