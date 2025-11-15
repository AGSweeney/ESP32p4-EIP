# VL53L0X ESP-IDF Component

This component wraps ST’s VL53L0X time-of-flight ranging sensor for modern ESP-IDF projects. It targets ESP-IDF v5.1+ and uses the `driver/i2c_master.h` API introduced in ESP-IDF v5.0.

## Features

- Works with ESP-IDF’s new I²C master bus/device handles.
- Handles sensor boot-up, timing-budget configuration, and the required reference calibration steps.
- Provides blocking single-shot measurements with optional multi-sample averaging.
- Exposes status codes so you can detect out-of-range conditions or invalid readings.
- Simple timing-budget API for trading speed vs. accuracy.

## Directory Layout

```
components/vl53l0x/
├── CMakeLists.txt
├── include/
│   └── vl53l0x.h         # Public API
├── vl53l0x.c             # Driver implementation
└── README.md             # This document
```

## Getting Started

1. **Add the component**

   Copy the `vl53l0x` folder into your project’s `components/` directory. In your project’s top-level `CMakeLists.txt`, include the directory if necessary:

   ```cmake
   set(EXTRA_COMPONENT_DIRS
       ${EXTRA_COMPONENT_DIRS}
       "${CMAKE_SOURCE_DIR}/components/vl53l0x")
   ```

2. **Wire the sensor**

   - SDA → ESP32 SDA pin (default in the driver example: GPIO5).
   - SCL → ESP32 SCL pin (default: GPIO6).
   - XSHUT (optional but recommended) → GPIO so you can reset the sensor.
   - VIN → 2.8–3.3 V, GND → ground.
   - Use 2.2 k–4.7 k pull-ups on SDA/SCL if the breakout does not include them.

3. **Enable I²C master driver**

   The driver expects you to create a master bus/device using the new API (`i2c_new_master_bus`, `i2c_master_bus_add_device`). See the example below.

## Public API (from `vl53l0x.h`)

```c
typedef struct {
    i2c_master_dev_handle_t i2c_dev;
    uint16_t timeout_ms;
} vl53l0x_config_t;

typedef struct {
    bool data_ready;
    bool range_valid;
    uint8_t raw_status;
    uint8_t range_status;
    bool out_of_range;
} vl53l0x_status_t;

bool vl53l0x_init(vl53l0x_t *dev, const vl53l0x_config_t *cfg);
bool vl53l0x_set_measurement_timing_budget(vl53l0x_t *dev, uint32_t budget_us);
uint32_t vl53l0x_get_measurement_timing_budget(vl53l0x_t *dev);
bool vl53l0x_read_range_single_mm(vl53l0x_t *dev, uint16_t *distance_mm,
                                  vl53l0x_status_t *status);
bool vl53l0x_get_status(vl53l0x_t *dev, vl53l0x_status_t *status);
```

## Example: Single-Shot Measurements with Averaging

```c
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "vl53l0x.h"

#define I2C_PORT          I2C_NUM_0
#define I2C_SCL_PIN       6
#define I2C_SDA_PIN       5
#define I2C_FREQ_HZ       400000
#define SAMPLE_COUNT      5
#define SAMPLE_INTERVAL_MS 5

static const char *TAG = "VL53L0X_DEMO";

void app_main(void)
{
    /* 1. Configure the master bus */
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .glitch_ignore_cnt = 7,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    /* 2. Attach the VL53L0X device */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L0X_I2C_ADDRESS_DEFAULT,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    i2c_master_dev_handle_t tof_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &tof_handle));

    /* 3. Initialise the driver */
    vl53l0x_t sensor;
    vl53l0x_config_t sensor_cfg = {
        .i2c_dev = tof_handle,
        .timeout_ms = 200,
    };
    if (!vl53l0x_init(&sensor, &sensor_cfg)) {
        ESP_LOGE(TAG, "VL53L0X init failed");
        return;
    }

    /* 4. Set a timing budget (100 ms is a good accuracy/speed trade-off) */
    vl53l0x_set_measurement_timing_budget(&sensor, 100000);

    TickType_t sample_delay = pdMS_TO_TICKS(SAMPLE_INTERVAL_MS);

    while (1) {
        uint32_t sum = 0;
        uint32_t valid = 0;
        vl53l0x_status_t last_status = {0};

        for (int i = 0; i < SAMPLE_COUNT; ++i) {
            uint16_t distance_mm = 0;
            vl53l0x_status_t status = {0};
            if (vl53l0x_read_range_single_mm(&sensor, &distance_mm, &status)) {
                last_status = status;
                if (status.range_valid && !status.out_of_range) {
                    sum += distance_mm;
                    ++valid;
                } else {
                    ESP_LOGW(TAG, "Invalid sample: raw=0x%02X range_status=%u",
                             status.raw_status, status.range_status);
                }
            } else {
                ESP_LOGW(TAG, "Measurement timeout");
            }
            vTaskDelay(sample_delay);
        }

        if (valid) {
            uint16_t avg_mm = sum / valid;
            ESP_LOGI(TAG, "Distance %u mm (status=0x%02X range_status=%u)",
                     avg_mm, last_status.raw_status, last_status.range_status);
        } else {
            ESP_LOGE(TAG, "No valid samples collected");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Notes on the Example

- The driver performs the required reference calibration during `vl53l0x_init`.
- `vl53l0x_set_measurement_timing_budget` takes microseconds. 33 ms is the sensor default; 50–200 ms smooths readings significantly.
- The averaging loop shows how to examine `vl53l0x_status_t` to ignore out-of-range samples.
- Use `i2c_master_bus_rm_device` and `i2c_del_master_bus` if you need to tear down the bus.

## Continuous Mode (Optional)

This component currently focuses on blocking single-shot reads. If you need continuous mode:

1. Write `0x02` to `SYSTEM_SEQUENCE_CONFIG` to enable continuous ranging stages.
2. Set `SYSTEM_INTERMEASUREMENT_PERIOD` to your desired interval.
3. Loop calling `vl53l0x_read_range_single_mm`; the driver already clears the interrupt flag so the next sample starts immediately.

You can also add helper functions in `vl53l0x.c` to wrap ST’s continuous-start/stop sequences if you need them frequently.

## Register Tuning

A quick recap of the registers worth adjusting:

- **Timing Budget:** `vl53l0x_set_measurement_timing_budget` (updates final/pre-range timeouts).
- **Signal Threshold:** `FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT` (0x44).
- **VCSEL Periods:** `PRE_RANGE_CONFIG_VCSEL_PERIOD` (0x50) and `FINAL_RANGE_CONFIG_VCSEL_PERIOD` (0x70).
- **Inter-Measurement Delay:** `SYSTEM_INTERMEASUREMENT_PERIOD` (0x04) when using continuous mode.
- **Offset / Crosstalk Compensation:** `ALGO_PART_TO_PART_RANGE_OFFSET_MM` (0x28), `CROSSTALK_COMPENSATION_PEAK_RATE_MCPS` (0x20).

See `REGISTERS.md` (if present) for a table explaining every register define used in the driver.

## Calibration

- The driver runs the built-in reference calibration each time you call `vl53l0x_init`.
- For application-specific calibration (offset, crosstalk), bring over the relevant routines from ST’s official VL53L0X API and call them after init while targeting known distances.

## Troubleshooting

| Symptom | Likely Cause / Fix |
| --- | --- |
| `Unexpected model id 0xC0` | Sensor hasn’t booted yet. Toggle XSHUT or delay ~10 ms before initialising. |
| Frequent `Range invalid ... range_status=4` | Target is beyond range; increase timing budget or check alignment. |
| `No valid samples` | All samples were invalid or timed out. Relax `FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT`, raise the timing budget, or ensure adequate illumination/reflectivity. |
| I²C warnings about deprecated API | Ensure you include `driver/i2c_master.h` and follow the new bus/device creation flow as shown above. |

## License

This component wraps ST’s VL53L0X API concepts with minimal logic of its own. Respect ST’s sensor license if you integrate additional code from their SDK. All code in this component adopts the same license as the parent project. 

