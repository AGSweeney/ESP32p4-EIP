# HX711 ESP-IDF Component

Minimal GPIO-based driver for SparkFun’s HX711 load-cell amplifier breakout. It implements blocking reads of the 24-bit ADC value and basic gain configuration.

## Directory Layout

```
components/hx711/
├── CMakeLists.txt
├── include/
│   └── hx711.h
└── hx711.c
```

## Public API

```c
typedef struct {
    int gpio_dout;
    int gpio_sck;
    hx711_gain_t gain;  // HX711_GAIN_128, HX711_GAIN_64, HX711_GAIN_32
} hx711_config_t;

esp_err_t hx711_init(hx711_t *dev, const hx711_config_t *cfg);
esp_err_t hx711_read_raw(hx711_t *dev, int32_t *value);
esp_err_t hx711_set_gain(hx711_t *dev, hx711_gain_t gain);
bool hx711_is_ready(hx711_t *dev);
esp_err_t hx711_wait_ready(hx711_t *dev, uint32_t timeout_ms);
esp_err_t hx711_read_average(hx711_t *dev, uint8_t samples, int32_t *value);
esp_err_t hx711_power_down(hx711_t *dev);
esp_err_t hx711_power_up(hx711_t *dev);
```

`hx711_read_raw` waits for DOUT to go low (with a 200 ms timeout) and then clocks out 24 bits, returning a sign-extended value. Use `hx711_wait_ready` if you need tighter control over the timeout, and `hx711_read_average` when you want a quick moving average.

## Example

```c
hx711_config_t cfg = {
    .gpio_dout = 4,
    .gpio_sck = 5,
    .gain = HX711_GAIN_128,
};

hx711_t amp;
ESP_ERROR_CHECK(hx711_init(&amp, &cfg));

int32_t sample = 0;
if (hx711_read_average(&amp, 8, &sample) == ESP_OK) {
    ESP_LOGI("HX711", "Averaged reading %ld", sample);
}

hx711_power_down(&amp);  // reduce consumption when idle
hx711_power_up(&amp);
```

Add your own calibration logic, averaging/filtering, and conversions to grams/pounds. The HX711 requires stable timing; ensure SCK pulses are clean and avoid other tasks blocking the loop for long periods.

