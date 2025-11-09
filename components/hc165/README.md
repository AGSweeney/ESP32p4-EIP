# 74HC165 ESP-IDF Component

Minimal helper library for the 74HC165 parallel-in shift register. It reads eight input bits (or more with cascading) using GPIO lines for the latch (SH/LD), clock (CP), and serial output (QH), plus an optional OE pin.

## Directory Layout

```
components/hc165/
├── CMakeLists.txt
├── include/
│   └── hc165.h
└── hc165.c
```

## Public API

```c
typedef struct {
    int gpio_data;   // QH serial output
    int gpio_clock;  // CP
    int gpio_latch;  // SH/LD (active low)
    int gpio_oe;     // optional OE (active low), set -1 if unused
    bool clock_idle_high;
    bool sample_on_falling_edge;
} hc165_config_t;

esp_err_t hc165_init(hc165_t *dev, const hc165_config_t *cfg);
esp_err_t hc165_shift_byte(hc165_t *dev, uint8_t *value);
esp_err_t hc165_shift_buffer(hc165_t *dev, uint8_t *data, size_t length);
esp_err_t hc165_set_output_enable(hc165_t *dev, bool enable);
```

- `hc165_init` sets up GPIO directions and default levels, copying your configuration.
- `hc165_shift_byte` latches the inputs and shifts them out MSB-first (configurable sampling edge).
- `hc165_shift_buffer` allows back-to-back reads for cascaded registers.
- `hc165_set_output_enable` controls the OE pin if you wire it to the MCU.

## Quick Example

```c
hc165_config_t cfg = {
    .gpio_data = 4,    // QH pin
    .gpio_clock = 5,
    .gpio_latch = 6,
    .gpio_oe = -1,
    .clock_idle_high = false,
    .sample_on_falling_edge = false,
};

hc165_t shiftreg;
ESP_ERROR_CHECK(hc165_init(&shiftreg, &cfg));

uint8_t inputs;
ESP_ERROR_CHECK(hc165_shift_byte(&shiftreg, &inputs));
ESP_LOGI("HC165", "Read bits: 0x%02X", inputs);
```

### Cascading

Chain the QH’ output into the next device’s SER input and use `hc165_shift_buffer`:

```c
uint8_t buffer[3];
hc165_shift_buffer(&shiftreg, buffer, 3);  // reads 24 bits MSB first
```

## Notes

- Pull-up resistors on the input pins help ensure stable readings.
- If your design clocks the register on the opposite edge, set `sample_on_falling_edge = true`.
- The latch (SH/LD) is active low; the library pulses it low/high before every read.
- OE should be held low to enable the output; use `hc165_set_output_enable` if you need to tri-state the output between reads.

