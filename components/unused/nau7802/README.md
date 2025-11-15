# NAU7802 ESP-IDF Component

Minimal helper for SparkFun's Qwiic Scale (NAU7802 24-bit load-cell ADC). Provides register read/write helpers and a conversion read routine; calibration, configuration, and continuous sampling modes can be layered on top.

## Directory Layout

```
components/nau7802/
├── CMakeLists.txt
├── include/
│   └── nau7802.h
└── nau7802.c
```

## Public API

```c
bool nau7802_init(nau7802_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t nau7802_write_register(nau7802_t *dev, uint8_t reg, uint8_t value);
esp_err_t nau7802_read_register(nau7802_t *dev, uint8_t reg, uint8_t *value);
esp_err_t nau7802_read_conversion(nau7802_t *dev, int32_t *value);
esp_err_t nau7802_soft_reset(nau7802_t *dev);
esp_err_t nau7802_power_up(nau7802_t *dev);
esp_err_t nau7802_power_down(nau7802_t *dev);
esp_err_t nau7802_set_gain(nau7802_t *dev, uint8_t gain);
esp_err_t nau7802_set_sample_rate(nau7802_t *dev, uint8_t rate);
esp_err_t nau7802_calibrate(nau7802_t *dev, bool internal_reference);
esp_err_t nau7802_is_data_ready(nau7802_t *dev, bool *ready);
esp_err_t nau7802_wait_ready(nau7802_t *dev, uint32_t timeout_ms);
```

`nau7802_read_conversion` fetches the 24-bit conversion result (sign-extended to 32 bits). Make sure the device is configured and new data is ready before calling it.

## Example

```c
nau7802_t scale;
nau7802_init(&scale, i2c_dev);
nau7802_soft_reset(&scale);
nau7802_power_up(&scale);
nau7802_set_gain(&scale, 0x03);        // gain x64
nau7802_set_sample_rate(&scale, 0x02); // 80 SPS
nau7802_calibrate(&scale, true);

if (nau7802_wait_ready(&scale, 200) == ESP_OK) {
    int32_t adc_counts = 0;
    if (nau7802_read_conversion(&scale, &adc_counts) == ESP_OK) {
        ESP_LOGI("NAU7802", "Raw counts = %ld", adc_counts);
    }
}
```

Refer to the NAU7802 datasheet or SparkFun library for calibration, offset, and power management routines. The helpers above cover reset, power control, gain/rate selection, calibration, and data-ready polling so you can compose your own weighing logic on top.

