# VL53L1X ESP-IDF Component

This component provides a comprehensive driver for the STMicroelectronics VL53L1X time-of-flight ranging sensor. The VL53L1X is an advanced ToF sensor offering improved performance over the VL53L0X with longer range (up to 4m), multiple distance modes, ROI (Region of Interest) configuration, and advanced calibration features.

## Features

- **Full VL53L1X API**: Complete implementation of all major sensor features
- **Multiple Distance Modes**: Short (up to 1.3m), Medium (up to 3m), and Long (up to 4m)
- **ROI Configuration**: Configure the region of interest for ranging measurements
- **Configurable Timing Budget**: Adjust measurement time vs. accuracy trade-off
- **Calibration Support**: Offset and crosstalk calibration functions
- **Interrupt Support**: Optional interrupt pin for measurement ready notification
- **Continuous Mode**: Support for continuous ranging measurements
- **ESP-IDF v5.x Compatible**: Uses modern `driver/i2c_master.h` API
- **Comprehensive Status Reporting**: Detailed measurement status and error codes

## Hardware Setup

### Pin Connections

- **VCC**: 2.8V - 3.3V power supply
- **GND**: Ground
- **SDA**: I2C data line (with pull-up resistor)
- **SCL**: I2C clock line (with pull-up resistor)
- **XSHUT** (optional): Hardware reset pin
- **GPIO1** (optional): Interrupt output pin

### I2C Address

- Default address: `0x29`
- Address can be changed via `vl53l1x_set_i2c_address()` (requires reset to take effect)

## Quick Start

### 1. Add Component

The component is already in your `components/` directory. Ensure your `CMakeLists.txt` includes it:

```cmake
set(EXTRA_COMPONENT_DIRS
    ${EXTRA_COMPONENT_DIRS}
    "${CMAKE_SOURCE_DIR}/components")
```

### 2. Initialize I2C Bus

```c
#include "driver/i2c_master.h"
#include "vl53l1x.h"

i2c_master_bus_handle_t bus_handle;
i2c_master_bus_config_t bus_cfg = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = GPIO_NUM_5,
    .scl_io_num = GPIO_NUM_6,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags = {
        .enable_internal_pullup = true,
    },
};
ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
```

### 3. Create I2C Device

```c
i2c_master_dev_handle_t dev_handle;
i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = VL53L1X_I2C_ADDRESS_DEFAULT,
    .scl_speed_hz = 400000,
};
ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
```

### 4. Initialize Sensor

```c
vl53l1x_t sensor;
vl53l1x_config_t sensor_cfg = {
    .i2c_dev = dev_handle,
    .timeout_ms = 200,
    .xshut_pin = GPIO_NUM_NC,  // Or GPIO pin if using hardware reset
    .enable_interrupt = false,  // Set to true if using interrupt pin
    .interrupt_pin = GPIO_NUM_NC,
    .int_polarity = VL53L1X_INTERRUPT_POLARITY_LOW,
};

if (!vl53l1x_init(&sensor, &sensor_cfg)) {
    ESP_LOGE(TAG, "VL53L1X initialization failed");
    return;
}
```

### 5. Read Distance

```c
uint16_t distance_mm;
if (vl53l1x_read_range_single_mm(&sensor, &distance_mm)) {
    ESP_LOGI(TAG, "Distance: %u mm", distance_mm);
} else {
    ESP_LOGE(TAG, "Measurement failed");
}
```

## API Reference

### Initialization

#### `vl53l1x_init()`

Initialize the VL53L1X sensor. Performs hardware reset (if XSHUT pin configured), firmware loading, calibration, and default configuration.

```c
bool vl53l1x_init(vl53l1x_t *dev, const vl53l1x_config_t *cfg);
```

**Parameters:**
- `dev`: Pointer to device handle structure
- `cfg`: Pointer to configuration structure

**Returns:** `true` on success, `false` on failure

### Distance Measurement

#### `vl53l1x_read_range_single_mm()`

Read a single distance measurement in millimeters (blocking).

```c
bool vl53l1x_read_range_single_mm(vl53l1x_t *dev, uint16_t *distance_mm);
```

**Parameters:**
- `dev`: Pointer to device handle structure
- `distance_mm`: Pointer to store distance in millimeters

**Returns:** `true` on success, `false` on failure

#### `vl53l1x_read_measurement()`

Read a complete measurement with all details including signal rate, ambient rate, and status.

```c
bool vl53l1x_read_measurement(vl53l1x_t *dev, vl53l1x_measurement_t *measurement);
```

**Parameters:**
- `dev`: Pointer to device handle structure
- `measurement`: Pointer to measurement result structure

**Returns:** `true` on success, `false` on failure

### Distance Modes

#### `vl53l1x_set_distance_mode()`

Set the distance measurement mode.

```c
bool vl53l1x_set_distance_mode(vl53l1x_t *dev, vl53l1x_distance_mode_t mode);
```

**Modes:**
- `VL53L1X_DISTANCE_MODE_SHORT`: Up to 1.3m, fastest measurements
- `VL53L1X_DISTANCE_MODE_MEDIUM`: Up to 3m, balanced performance
- `VL53L1X_DISTANCE_MODE_LONG`: Up to 4m, slowest but most accurate

**Example:**
```c
vl53l1x_set_distance_mode(&sensor, VL53L1X_DISTANCE_MODE_MEDIUM);
```

### Timing Budget

#### `vl53l1x_set_timing_budget()`

Set the measurement timing budget in microseconds. Longer budgets provide better accuracy but slower measurements.

```c
bool vl53l1x_set_timing_budget(vl53l1x_t *dev, uint32_t budget_us);
```

**Valid Range:** 15000 (15ms) to 1000000 (1000ms)

**Example:**
```c
vl53l1x_set_timing_budget(&sensor, VL53L1X_TIMING_BUDGET_100MS);
```

### Region of Interest (ROI)

#### `vl53l1x_set_roi()`

Set the Region of Interest using corner coordinates.

```c
bool vl53l1x_set_roi(vl53l1x_t *dev, const vl53l1x_roi_t *roi);
```

**Example:**
```c
vl53l1x_roi_t roi = {
    .top_left_x = 4,
    .top_left_y = 4,
    .bottom_right_x = 11,
    .bottom_right_y = 11,
};
vl53l1x_set_roi(&sensor, &roi);
```

#### `vl53l1x_set_roi_center()`

Set ROI using center point and size (convenience function).

```c
bool vl53l1x_set_roi_center(vl53l1x_t *dev, uint8_t center_x, uint8_t center_y, uint8_t size);
```

**Example:**
```c
// Set 8x8 ROI centered at (8, 8)
vl53l1x_set_roi_center(&sensor, 8, 8, 8);
```

### Calibration

#### `vl53l1x_calibrate_offset()`

Perform offset calibration. Place a flat, reflective target at the specified distance.

```c
bool vl53l1x_calibrate_offset(vl53l1x_t *dev, uint16_t target_distance_mm);
```

**Example:**
```c
// Calibrate with target at 200mm
vl53l1x_calibrate_offset(&sensor, 200);
```

#### `vl53l1x_calibrate_xtalk()`

Perform crosstalk calibration. Place a flat, reflective target at close range (typically 100mm).

```c
bool vl53l1x_calibrate_xtalk(vl53l1x_t *dev, uint16_t target_distance_mm);
```

**Example:**
```c
// Calibrate crosstalk with target at 100mm
vl53l1x_calibrate_xtalk(&sensor, 100);
```

### Continuous Mode

#### `vl53l1x_start_continuous()`

Start continuous ranging measurements.

```c
bool vl53l1x_start_continuous(vl53l1x_t *dev, uint32_t period_ms);
```

**Parameters:**
- `dev`: Pointer to device handle structure
- `period_ms`: Inter-measurement period in milliseconds (must be >= timing budget)

**Example:**
```c
// Start continuous mode with 100ms period
vl53l1x_start_continuous(&sensor, 100);

// Read measurements in loop
while (1) {
    vl53l1x_measurement_t measurement;
    if (vl53l1x_read_measurement(&sensor, &measurement)) {
        if (measurement.range_valid) {
            ESP_LOGI(TAG, "Distance: %u mm", measurement.distance_mm);
        }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}
```

#### `vl53l1x_stop_continuous()`

Stop continuous ranging measurements.

```c
bool vl53l1x_stop_continuous(vl53l1x_t *dev);
```

### Interrupt Support

To use interrupts, configure the interrupt pin in the initialization:

```c
vl53l1x_config_t sensor_cfg = {
    .i2c_dev = dev_handle,
    .timeout_ms = 200,
    .enable_interrupt = true,
    .interrupt_pin = GPIO_NUM_7,
    .int_polarity = VL53L1X_INTERRUPT_POLARITY_LOW,
};
```

Then check for data ready:

```c
bool ready = false;
if (vl53l1x_check_data_ready(&sensor, &ready) && ready) {
    vl53l1x_measurement_t measurement;
    if (vl53l1x_read_measurement(&sensor, &measurement)) {
        // Process measurement
    }
    vl53l1x_clear_interrupt(&sensor);
}
```

## Complete Example

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "vl53l1x.h"

static const char *TAG = "vl53l1x_example";

void vl53l1x_example_task(void *pvParameters)
{
    /* Initialize I2C bus */
    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_5,
        .scl_io_num = GPIO_NUM_6,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    /* Create I2C device */
    i2c_master_dev_handle_t dev_handle;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L1X_I2C_ADDRESS_DEFAULT,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    /* Initialize sensor */
    vl53l1x_t sensor;
    vl53l1x_config_t sensor_cfg = {
        .i2c_dev = dev_handle,
        .timeout_ms = 200,
        .xshut_pin = GPIO_NUM_NC,
        .enable_interrupt = false,
        .interrupt_pin = GPIO_NUM_NC,
        .int_polarity = VL53L1X_INTERRUPT_POLARITY_LOW,
    };

    if (!vl53l1x_init(&sensor, &sensor_cfg)) {
        ESP_LOGE(TAG, "Initialization failed");
        vTaskDelete(NULL);
        return;
    }

    /* Configure sensor */
    vl53l1x_set_distance_mode(&sensor, VL53L1X_DISTANCE_MODE_MEDIUM);
    vl53l1x_set_timing_budget(&sensor, VL53L1X_TIMING_BUDGET_100MS);

    /* Main measurement loop */
    while (1) {
        vl53l1x_measurement_t measurement;
        if (vl53l1x_read_measurement(&sensor, &measurement)) {
            if (measurement.range_valid) {
                ESP_LOGI(TAG, "Distance: %u mm, Signal: %u kcps, Status: %d",
                         measurement.distance_mm,
                         measurement.signal_rate_kcps,
                         measurement.status);
            } else {
                ESP_LOGW(TAG, "Invalid measurement, status: %d", measurement.status);
            }
        } else {
            ESP_LOGE(TAG, "Measurement failed");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## Measurement Status Codes

The `vl53l1x_measurement_t` structure includes a `status` field with the following possible values:

- `VL53L1X_RANGE_STATUS_OK`: Valid range measurement
- `VL53L1X_RANGE_STATUS_SIGMA_FAIL`: Sigma estimator check failed
- `VL53L1X_RANGE_STATUS_SIGNAL_FAIL`: Signal strength check failed
- `VL53L1X_RANGE_STATUS_OUT_OF_BOUNDS_FAIL`: Target out of bounds
- `VL53L1X_RANGE_STATUS_HARDWARE_FAIL`: Hardware failure
- `VL53L1X_RANGE_STATUS_RANGE_VALID_MIN`: Range valid but below minimum
- `VL53L1X_RANGE_STATUS_RANGE_VALID_MAX`: Range valid but above maximum
- `VL53L1X_RANGE_STATUS_WRAP_TARGET_FAIL`: Wrapped target fail
- `VL53L1X_RANGE_STATUS_PROCESSING_FAIL`: Processing failure
- `VL53L1X_RANGE_STATUS_XTALK_SIGNAL_FAIL`: Xtalk signal check failed
- `VL53L1X_RANGE_STATUS_UPDATE_FAIL`: Update failure
- `VL53L1X_RANGE_STATUS_NO_UPDATE`: No update

## Notes

- The sensor requires proper power supply (2.8V - 3.3V) and I2C pull-up resistors (typically 2.2kΩ - 4.7kΩ)
- For best accuracy, perform offset and crosstalk calibration in your application environment
- Timing budget affects both measurement time and accuracy - longer budgets provide better accuracy
- ROI configuration can improve performance by focusing on a specific area of the field of view
- Multiple sensors can be used on the same I2C bus by changing their addresses via `vl53l1x_set_i2c_address()`

## References

- [VL53L1X Datasheet](https://www.st.com/resource/en/datasheet/vl53l1x.pdf)
- [VL53L1X API User Manual](https://www.st.com/resource/en/user_manual/um2556-vl53l1x-api-user-manual-stmicroelectronics.pdf)

## License

This driver is provided as-is for use with ESP-IDF projects.

