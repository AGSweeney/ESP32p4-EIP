# LSM6DSV16X ESP-IDF Component

Scaffold for the SparkFun 6DoF IMU Breakout (ST LSM6DSV16X). This layer exposes I²C register helpers and a basic configuration routine for quick bring-up inside ESP-IDF projects.

## Directory Layout

```
components/lsm6dsv16x/
├── CMakeLists.txt
├── include/
│   └── lsm6dsv16x.h
└── lsm6dsv16x.c
```

## Public API

```c
bool lsm6dsv16x_init(lsm6dsv16x_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t lsm6dsv16x_write_register(lsm6dsv16x_t *dev, uint8_t reg, uint8_t value);
esp_err_t lsm6dsv16x_read_register(lsm6dsv16x_t *dev, uint8_t reg, uint8_t *value);
esp_err_t lsm6dsv16x_read_bytes(lsm6dsv16x_t *dev, uint8_t reg, uint8_t *buffer, size_t length);
esp_err_t lsm6dsv16x_configure_default(lsm6dsv16x_t *dev);
esp_err_t lsm6dsv16x_soft_reset(lsm6dsv16x_t *dev);
esp_err_t lsm6dsv16x_read_id(lsm6dsv16x_t *dev, uint8_t *who_am_i);
esp_err_t lsm6dsv16x_set_accel_config(lsm6dsv16x_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t lsm6dsv16x_set_gyro_config(lsm6dsv16x_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t lsm6dsv16x_read_sample(lsm6dsv16x_t *dev, lsm6dsv16x_sample_t *sample);
```

`lsm6dsv16x_configure_default` enables accelerometer and gyroscope at moderate data rates and full-scale settings. Adjust the register programming to match your bandwidth and sensitivity requirements.

## Example

```c
lsm6dsv16x_t imu;
lsm6dsv16x_init(&imu, i2c_dev);
ESP_ERROR_CHECK(lsm6dsv16x_configure_default(&imu));

uint8_t who_am_i = 0;
ESP_ERROR_CHECK(lsm6dsv16x_read_register(&imu, 0x0F, &who_am_i));
ESP_LOGI("LSM6DSV16X", "WHOAMI 0x%02X", who_am_i);

lsm6dsv16x_sample_t sample;
if (lsm6dsv16x_read_sample(&imu, &sample) == ESP_OK) {
    ESP_LOGI("LSM6DSV16X", "Accel %d %d %d Gyro %d %d %d",
             sample.accel_x, sample.accel_y, sample.accel_z,
             sample.gyro_x, sample.gyro_y, sample.gyro_z);
}
```

## Notes

- SparkFun boards expose both primary (0x6A) and secondary (0x6B) I²C addresses depending on SA0 pin state.
- The sensor supports significant processing (OIS, embedded filtering, machine learning cores). Layer those features on top of these primitives as needed.
- For highest performance, consider using the FIFO or interrupts instead of polling.

