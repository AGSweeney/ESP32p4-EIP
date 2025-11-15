# BMI270 ESP-IDF Component

Helper layer for the SparkFun 6DoF IMU Breakout (Bosch BMI270). This scaffold exposes read and write primitives and a minimal configuration routine so you can integrate custom motion processing on top of ESP-IDF.

## Directory Layout

```
components/bmi270/
├── CMakeLists.txt
├── include/
│   └── bmi270.h
└── bmi270.c
```

## Public API

```c
bool bmi270_init(bmi270_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t bmi270_write_register(bmi270_t *dev, uint8_t reg, uint8_t value);
esp_err_t bmi270_read_register(bmi270_t *dev, uint8_t reg, uint8_t *value);
esp_err_t bmi270_read_bytes(bmi270_t *dev, uint8_t reg, uint8_t *buffer, size_t length);
esp_err_t bmi270_configure_default(bmi270_t *dev);
esp_err_t bmi270_soft_reset(bmi270_t *dev);
esp_err_t bmi270_read_chip_id(bmi270_t *dev, uint8_t *chip_id);
esp_err_t bmi270_set_accel_config(bmi270_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t bmi270_set_gyro_config(bmi270_t *dev, uint8_t odr_bits, uint8_t range_bits, uint8_t filter_bits);
esp_err_t bmi270_enable_sensors(bmi270_t *dev, bool accel_enable, bool gyro_enable);
esp_err_t bmi270_read_sample(bmi270_t *dev, bmi270_sample_t *sample);
```

`bmi270_configure_default` performs a soft reset and pushes a simple accelerometer/gyroscope configuration suitable for baseline testing. Extend or replace it with the device configuration stream recommended by Bosch for production.

## Example

```c
bmi270_t imu;
bmi270_init(&imu, i2c_dev);
ESP_ERROR_CHECK(bmi270_configure_default(&imu));

uint8_t chip_id = 0;
ESP_ERROR_CHECK(bmi270_read_chip_id(&imu, &chip_id));
ESP_LOGI("BMI270", "Chip ID 0x%02X", chip_id);

bmi270_sample_t sample;
if (bmi270_read_sample(&imu, &sample) == ESP_OK) {
    ESP_LOGI("BMI270", "Accel %d %d %d Gyro %d %d %d Temp %d",
             sample.accel_x, sample.accel_y, sample.accel_z,
             sample.gyro_x, sample.gyro_y, sample.gyro_z,
             sample.temperature);
}
```

## Notes

- The BMI270 expects a configuration file streamed into the device for full operation. Consult SparkFun’s or Bosch’s reference projects for advanced motion detection features.
- The device supports both I²C and SPI. This component targets I²C via ESP-IDF’s `i2c_master` API.
- Tune power modes, IRQ routing, and FIFO usage based on your application’s latency and power constraints.

## Streaming the BMI270 Configuration File

Bosch ships a binary “configuration file” (also called the feature configuration or firmware image) that must be downloaded into the BMI270’s internal memory before enabling many of the higher-level motion features (step detection, wrist wear, etc.). You can obtain the official blob from Bosch’s public repository: <https://github.com/BoschSensortec/BMI270-Sensor-API> (see the `config_file` directory). The download sequence is:

1. Place the device into config-loading mode (`BMI270_REG_INIT_CTRL` = `0x00`).
2. Stream the configuration blob into `BMI270_REG_INIT_DATA` (0x5E) using bursts of up to 16 bytes.
3. Exit config-loading mode (`BMI270_REG_INIT_CTRL` = `0x01`) and wait for `INIT_STATUS` to report success.

### Template

Create a C source file alongside your application (or inside `components/bmi270/`) containing the configuration bytes. The snippet below uses a placeholder array; replace the contents with the official binary from Bosch (typically supplied as `BMI270_config_file.h` in their reference driver):

```c
#include "bmi270.h"

static const uint8_t bmi270_config_file[] = {
    /* Replace with the official configuration blob.
       Bosch distributes a ~6 KB array. For example purposes:
       0x01, 0x03, 0x03, 0x0E, 0x1B, 0x00, 0x02, 0x06,
       ... (rest of the configuration bytes) ...
    */
};

static const size_t bmi270_config_file_size = sizeof(bmi270_config_file);
```

### Loader Helper

Add a helper that pushes the array into the device. The configuration interface uses `INIT_ADDR_0`/`INIT_ADDR_1` (0x59/0x5A) to set the write pointer and `INIT_DATA` (0x5E) to stream data.

```c
static esp_err_t bmi270_write_bytes(bmi270_t *imu,
                                    uint8_t reg,
                                    const uint8_t *data,
                                    size_t length)
{
    if (!imu || !data || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t payload[17];
    payload[0] = reg;
    for (size_t i = 0; i < length; ++i) {
        payload[i + 1] = data[i];
    }
    return i2c_master_transmit(imu->i2c_dev,
                               payload,
                               length + 1,
                               pdMS_TO_TICKS(100));
}

static esp_err_t bmi270_upload_config(bmi270_t *imu,
                                      const uint8_t *config,
                                      size_t length)
{
    if (!imu || !config || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Put BMI270 into config loading mode
    ESP_RETURN_ON_ERROR(bmi270_write_register(imu, 0x59, 0x00), TAG, "addr0");
    ESP_RETURN_ON_ERROR(bmi270_write_register(imu, 0x5A, 0x00), TAG, "addr1");
    ESP_RETURN_ON_ERROR(bmi270_write_register(imu, 0x5B, 0x00), TAG, "cfg mode");

    size_t offset = 0;
    while (offset < length) {
        size_t chunk = length - offset;
        if (chunk > 16) {
            chunk = 16;  // datasheet recommends 2–16 bytes per burst
        }
        ESP_RETURN_ON_ERROR(
            bmi270_write_bytes(imu, 0x5E, &config[offset], chunk),
            TAG, "cfg chunk");
        offset += chunk;
    }

    // Exit config loading mode
    ESP_RETURN_ON_ERROR(bmi270_write_register(imu, 0x5B, 0x01), TAG, "cfg exit");

    uint8_t status = 0;
    int retries = 50;
    while (retries--) {
        ESP_RETURN_ON_ERROR(bmi270_read_register(imu, 0x21, &status), TAG, "status");
        if (status & 0x01) {
            return ESP_OK;  // init done
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return ESP_ERR_TIMEOUT;
}
```

### Putting It Together

```c
bmi270_t imu;
bmi270_init(&imu, i2c_dev);
ESP_ERROR_CHECK(bmi270_soft_reset(&imu));
ESP_ERROR_CHECK(bmi270_upload_config(&imu,
                                     bmi270_config_file,
                                     bmi270_config_file_size));
ESP_ERROR_CHECK(bmi270_enable_sensors(&imu, true, true));
```

- Always verify that the configuration file you stream matches the BMI270 revision on your board.
- If you update to a newer Bosch config package, re-run the streaming procedure after the next reset.

### Computing Euler Angles (Roll/Pitch/Yaw)

Use the complementary-filter helpers to derive Euler angles directly from the raw accelerometer and gyroscope readings. Supply the correct scale factors for the ranges you configured and the sampling interval (`dt`).

```c
#include <math.h>

// Example scale factors for ±2 g accelerometer and ±2000 dps gyroscope
static const float ACCEL_G_PER_LSB = 0.000061f;     // 2 g / 32768
static const float GYRO_DPS_PER_LSB = 0.061035f;    // 2000 dps / 32768
static const float RAD_TO_DEG = 57.2957795f;

bmi270_orientation_state_t orientation_state;
bmi270_orientation_init(&orientation_state, 0.98f); // alpha ∈ [0,1]

const float dt_sec = 0.01f; // 100 Hz loop
while (true)
{
    bmi270_sample_t sample;
    if (bmi270_read_sample(&imu, &sample) != ESP_OK)
    {
        continue;
    }

    bmi270_euler_t euler = {0};
    if (bmi270_orientation_update(&orientation_state,
                                  &sample,
                                  GYRO_DPS_PER_LSB,
                                  ACCEL_G_PER_LSB,
                                  dt_sec,
                                  &euler) == ESP_OK)
    {
        float roll_deg = euler.roll * RAD_TO_DEG;
        float pitch_deg = euler.pitch * RAD_TO_DEG;
        float yaw_deg = euler.yaw * RAD_TO_DEG; // yaw drifts without magnetometer correction
        ESP_LOGI("BMI270", "Euler roll=%.2f pitch=%.2f yaw=%.2f deg",
                 roll_deg, pitch_deg, yaw_deg);
    }
    vTaskDelay(pdMS_TO_TICKS((int)(dt_sec * 1000)));
}
```

- `alpha` (0–1) biases the complementary filter toward the gyro (higher) or accelerometer (lower). Typical values: 0.90–0.99.
- Update `ACCEL_G_PER_LSB` and `GYRO_DPS_PER_LSB` if you change the configured sensor ranges.
- Yaw uses only integrated gyroscope data; expect drift unless you add a magnetometer or external reference.

## Notes

- The BMI270 expects a configuration file streamed into the device for full operation. Consult SparkFun’s or Bosch’s reference projects for advanced motion detection features.
- The device supports both I²C and SPI. This component targets I²C via ESP-IDF’s `i2c_master` API.
- Tune power modes, IRQ routing, and FIFO usage based on your application’s latency and power constraints.

