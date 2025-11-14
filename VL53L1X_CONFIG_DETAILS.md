# VL53L1x Configuration Parameters - Detailed Reference

This document provides detailed explanations of each VL53L1x configuration parameter for the Web UI implementation.

## Configuration Structure

```c
typedef struct {
    // Distance Mode Configuration
    uint16_t distance_mode;           // 1=SHORT (<1.3m), 2=LONG (<4m, default)
    uint16_t timing_budget_ms;        // 15, 20, 33, 50, 100 (default), 200, 500
    uint32_t inter_measurement_ms;   // Must be >= timing_budget, default: 100
    
    // Region of Interest (ROI) Configuration
    uint16_t roi_x_size;              // 4-16 (default: 16)
    uint16_t roi_y_size;              // 4-16 (default: 16)
    uint8_t roi_center_spad;          // 0-199 (default: 199 = center)
    
    // Calibration Values
    int16_t offset_mm;                // -128 to +127 mm (default: 0)
    uint16_t xtalk_cps;               // 0-65535 cps (default: 0)
    
    // Threshold Configuration
    uint16_t signal_threshold_kcps;   // 0-65535 kcps (default: 1024)
    uint16_t sigma_threshold_mm;      // 0-65535 mm (default: 15)
    
    // Distance Threshold Detection (for interrupts)
    uint16_t threshold_low_mm;        // 0-4000 mm (default: 0 = disabled)
    uint16_t threshold_high_mm;       // 0-4000 mm (default: 0 = disabled)
    uint8_t threshold_window;         // 0=below, 1=above, 2=out, 3=in (default: 0)
    
    // Interrupt Configuration
    uint8_t interrupt_polarity;        // 0=active low, 1=active high (default: 1)
    
    // I2C Configuration (for multiple sensors)
    uint8_t i2c_address;              // 0x29-0x7F (default: 0x29)
} vl53l1x_config_t;
```

## Parameter Descriptions

### 1. distance_mode (uint16_t)

**Purpose**: Selects measurement range and performance characteristics

**Values**:
- `1` = SHORT mode
- `2` = LONG mode (default)

**SHORT Mode**:
- **Maximum range**: 1.3 meters
- **Performance**: Better ambient light immunity, faster measurement cycles
- **Update rate**: Up to 50-100 Hz (depending on timing budget)
- **Best for**: Indoor applications, close-range detection (<1.3m), high ambient light environments
- **Trade-off**: Limited range but better noise immunity

**LONG Mode**:
- **Maximum range**: 4 meters (in dark conditions with 200ms timing budget)
- **Performance**: Optimized for longer distances
- **Update rate**: Up to 30 Hz (depending on timing budget)
- **Best for**: Outdoor applications, longer ranges (1-4m), lower ambient light
- **Trade-off**: Longer range but more susceptible to ambient light

**API Functions**:
- `VL53L1X_SetDistanceMode(dev, mode)`
- `VL53L1X_GetDistanceMode(dev, &mode)`

---

### 2. timing_budget_ms (uint16_t)

**Purpose**: Sets the time allocated for each measurement cycle

**Valid Values**: `15`, `20`, `33`, `50`, `100`, `200`, `500` milliseconds

**Default**: `100` ms

**Trade-offs**:
- **Shorter timing budget (15-33ms)**:
  - Faster measurement updates
  - Lower measurement accuracy
  - Higher power consumption
  - More susceptible to noise
  - Best for: High-speed applications where approximate distance is acceptable

- **Longer timing budget (100-500ms)**:
  - Slower measurement updates
  - Higher measurement accuracy
  - Better noise immunity
  - Lower power consumption per measurement
  - Best for: Applications requiring high accuracy

**Important**: Inter-measurement period must be >= timing budget

**API Functions**:
- `VL53L1X_SetTimingBudgetInMs(dev, budget)`
- `VL53L1X_GetTimingBudgetInMs(dev, &budget)`

---

### 3. inter_measurement_ms (uint32_t)

**Purpose**: Controls the delay between consecutive measurements in continuous ranging mode

**Range**: Must be >= timing_budget_ms

**Default**: `100` ms

**Effect**: 
- Lower values = higher update rate (up to sensor physical limits)
- Must be at least equal to timing_budget_ms
- Setting too low will cause measurements to overlap

**Example**:
- Timing budget: 100ms, Inter-measurement: 100ms → 10 Hz update rate
- Timing budget: 33ms, Inter-measurement: 33ms → 30 Hz update rate
- Timing budget: 20ms, Inter-measurement: 20ms → 50 Hz update rate

**API Functions**:
- `VL53L1X_SetInterMeasurementInMs(dev, period)`
- `VL53L1X_GetInterMeasurementInMs(dev, &period)`

---

### 4. roi_x_size (uint16_t)

**Purpose**: Sets the Region of Interest (ROI) width (horizontal field of view)

**Range**: `4` to `16` (minimum 4, maximum 16)

**Default**: `16` (full width)

**Effect**:
- **Smaller ROI (4-8)**: Faster measurements, narrower field of view, less ambient light collection
- **Larger ROI (12-16)**: Slower measurements, wider field of view, more ambient light collection

**Use Cases**:
- Small ROI: Precise targeting, faster updates, reduced ambient interference
- Large ROI: Wide area detection, better signal collection, slower updates

**API Functions**:
- `VL53L1X_SetROI(dev, x_size, y_size)`
- `VL53L1X_GetROI_XY(dev, &x_size, &y_size)`

---

### 5. roi_y_size (uint16_t)

**Purpose**: Sets the Region of Interest (ROI) height (vertical field of view)

**Range**: `4` to `16` (minimum 4, maximum 16)

**Default**: `16` (full height)

**Effect**: Same trade-offs as `roi_x_size` but for vertical dimension

**Note**: ROI size affects the number of active SPADs, which impacts measurement speed and accuracy

**API Functions**:
- `VL53L1X_SetROI(dev, x_size, y_size)`
- `VL53L1X_GetROI_XY(dev, &x_size, &y_size)`

---

### 6. roi_center_spad (uint8_t)

**Purpose**: Fine-tunes the center point of the Region of Interest

**Range**: `0` to `199` (SPAD array indices)

**Default**: `199` (center of SPAD array)

**Effect**: Allows precise targeting of a specific measurement area within the sensor's field of view

**Important**: 
- ROI center must be within bounds of the ROI size
- If ROI center + ROI size exceeds SPAD array bounds, measurement will fail (status error 13)

**Use Cases**:
- Off-center targeting for specific applications
- Fine-tuning measurement point
- Compensating for sensor mounting angle

**API Functions**:
- `VL53L1X_SetROICenter(dev, center_spad)`
- `VL53L1X_GetROICenter(dev, &center_spad)`

---

### 7. offset_mm (int16_t)

**Purpose**: Compensates for systematic distance measurement errors

**Range**: `-128` to `+127` millimeters (signed)

**Default**: `0` (no offset compensation)

**Calibration Procedure**:
1. Place a target at a known distance (ST recommends 100mm)
2. Use a grey 17% reflectance target
3. Stop ranging: `VL53L1X_StopRanging(dev)`
4. Call: `VL53L1X_CalibrateOffset(dev, 100, &offset)`
5. Sensor automatically applies the calculated offset
6. Restart ranging: `VL53L1X_StartRanging(dev)`

**Usage**:
- **Positive value**: Sensor reads too close → offset increases reading
- **Negative value**: Sensor reads too far → offset decreases reading

**When to Recalibrate**:
- Sensor replacement
- Significant environment changes (temperature, humidity)
- Cover glass changes
- After extended period of non-use

**API Functions**:
- `VL53L1X_SetOffset(dev, offset_mm)`
- `VL53L1X_GetOffset(dev, &offset_mm)`
- `VL53L1X_CalibrateOffset(dev, target_distance_mm, &offset_mm)`

---

### 8. xtalk_cps (uint16_t)

**Purpose**: Compensates for photons reflected from cover glass (crosstalk)

**Range**: `0` to `65535` counts per second (cps)

**Default**: `0` (no crosstalk compensation)

**Calibration Procedure**:
1. Place a target at the inflection point (typically ~600mm)
2. Use a grey 17% reflectance target
3. Stop ranging: `VL53L1X_StopRanging(dev)`
4. Call: `VL53L1X_CalibrateXtalk(dev, 600, &xtalk)`
5. Sensor automatically applies the calculated xtalk value
6. Restart ranging: `VL53L1X_StartRanging(dev)`

**Effect**: 
- Important for accurate measurements at medium distances (300-800mm)
- Reduces "under-ranging" caused by cover glass reflections
- Improves measurement accuracy in the critical medium-range zone

**When to Recalibrate**:
- Cover glass replacement
- Significant environment changes
- After noticing measurement errors at medium distances

**API Functions**:
- `VL53L1X_SetXtalk(dev, xtalk_cps)`
- `VL53L1X_GetXtalk(dev, &xtalk_cps)`
- `VL53L1X_CalibrateXtalk(dev, target_distance_mm, &xtalk_cps)`

---

### 9. signal_threshold_kcps (uint16_t)

**Purpose**: Sets minimum signal strength threshold for valid measurements

**Range**: `0` to `65535` kilo counts per second (kcps)

**Default**: `1024` kcps (1,024,000 counts per second)

**Units**: Kilo counts per second (kcps = 1000 counts per second)

**Effect**:
- **Lower values (256-512 kcps)**: 
  - More sensitive to weak signals
  - May include noise in measurements
  - Detects weak or distant targets
  - May produce false readings

- **Higher values (2048-4096 kcps)**:
  - Less sensitive, filters out noise
  - More reliable measurements
  - May miss weak or distant targets
  - Better for strong, close targets

**Use Cases**:
- High ambient light: Increase threshold to filter noise
- Weak targets: Decrease threshold to detect them
- Strong targets: Increase threshold for reliability

**API Functions**:
- `VL53L1X_SetSignalThreshold(dev, threshold_kcps)`
- `VL53L1X_GetSignalThreshold(dev, &threshold_kcps)`

---

### 10. sigma_threshold_mm (uint16_t)

**Purpose**: Sets maximum measurement uncertainty threshold

**Range**: `0` to `65535` millimeters

**Default**: `15` mm

**Units**: Millimeters (measurement uncertainty/standard deviation)

**Effect**:
- **Lower values (5-10 mm)**:
  - Stricter quality requirements
  - Rejects noisy measurements
  - Higher measurement reliability
  - May reject valid measurements in noisy conditions

- **Higher values (20-30 mm)**:
  - More lenient quality requirements
  - Accepts noisier measurements
  - More measurements accepted
  - May include less reliable readings

**Use Cases**:
- Stable conditions: Lower threshold for high quality
- Noisy conditions: Higher threshold to accept more measurements
- High-speed applications: Higher threshold to avoid rejecting valid fast measurements

**API Functions**:
- `VL53L1X_SetSigmaThreshold(dev, threshold_mm)`
- `VL53L1X_GetSigmaThreshold(dev, &threshold_mm)`

---

### 11. threshold_low_mm (uint16_t)

**Purpose**: Low distance threshold for interrupt-based detection

**Range**: `0` to `4000` millimeters

**Default**: `0` (disabled)

**Usage**: Used with `threshold_window` to generate interrupts when distance crosses threshold

**Example**: 
- `threshold_low_mm = 100`, `threshold_window = 0` → Interrupt when distance < 100mm

**API Functions**:
- `VL53L1X_SetDistanceThreshold(dev, low_mm, high_mm, window, int_on_no_target)`
- `VL53L1X_GetDistanceThresholdLow(dev, &low_mm)`

---

### 12. threshold_high_mm (uint16_t)

**Purpose**: High distance threshold for interrupt-based detection

**Range**: `0` to `4000` millimeters

**Default**: `0` (disabled)

**Constraint**: Must be >= `threshold_low_mm`

**Usage**: Used with `threshold_window` for window-based detection

**Example**:
- `threshold_low_mm = 100`, `threshold_high_mm = 300`, `threshold_window = 3` → Interrupt when 100mm <= distance <= 300mm

**API Functions**:
- `VL53L1X_SetDistanceThreshold(dev, low_mm, high_mm, window, int_on_no_target)`
- `VL53L1X_GetDistanceThresholdHigh(dev, &high_mm)`

---

### 13. threshold_window (uint8_t)

**Purpose**: Configures distance threshold detection window mode

**Values**:
- `0` = Below threshold (interrupt when distance < threshold_low)
- `1` = Above threshold (interrupt when distance > threshold_high)
- `2` = Out of window (interrupt when distance < low OR distance > high)
- `3` = In window (interrupt when low <= distance <= high)

**Default**: `0` (disabled if thresholds are 0)

**Use Cases**:
- **Mode 0 (Below)**: Object detection, proximity warning
- **Mode 1 (Above)**: Distance monitoring, object removal detection
- **Mode 2 (Out)**: Range monitoring, object entry/exit detection
- **Mode 3 (In)**: Zone detection, object presence in range

**API Functions**:
- `VL53L1X_SetDistanceThreshold(dev, low_mm, high_mm, window, int_on_no_target)`
- `VL53L1X_GetDistanceThresholdWindow(dev, &window)`

---

### 14. interrupt_polarity (uint8_t)

**Purpose**: Configures GPIO interrupt pin polarity

**Values**:
- `0` = Active low (interrupt pin goes LOW when triggered)
- `1` = Active high (interrupt pin goes HIGH when triggered)

**Default**: `1` (active high)

**Usage**: Must match hardware interrupt pin configuration

**Hardware Consideration**: 
- Check if interrupt GPIO has pull-up or pull-down resistor
- Match polarity to hardware configuration

**API Functions**:
- `VL53L1X_SetInterruptPolarity(dev, polarity)`
- `VL53L1X_GetInterruptPolarity(dev, &polarity)`

---

### 15. i2c_address (uint8_t)

**Purpose**: I2C address for multiple sensors on the same bus

**Range**: `0x29` to `0x7F` (valid I2C 7-bit addresses)

**Default**: `0x29`

**Usage**: 
- Change address using XSHUT pin during initialization
- Allows up to 16 sensors on one I2C bus (with different addresses)
- Address change is persistent until power cycle

**Procedure for Multiple Sensors**:
1. Initialize first sensor at default address (0x29)
2. Use XSHUT pin to reset second sensor
3. Change address of second sensor to new value (e.g., 0x30)
4. Release XSHUT pin
5. Initialize second sensor at new address

**API Functions**:
- `VL53L1X_SetI2CAddress(dev, new_address)`
- `vl53l1x_update_device_address(device, new_address)`

---

## Configuration Functions

### Load Configuration
```c
bool vl53l1x_config_load(vl53l1x_config_t *config);
```
- Loads configuration from NVS storage
- Returns `true` on success, `false` if no saved config exists or error occurred
- If no saved config exists, config structure remains unchanged

### Save Configuration
```c
bool vl53l1x_config_save(const vl53l1x_config_t *config);
```
- Saves configuration to NVS storage
- Returns `true` on success, `false` on error
- Configuration persists across reboots

### Get Defaults
```c
void vl53l1x_config_get_defaults(vl53l1x_config_t *config);
```
- Initializes config structure with default values:
  - distance_mode: 2 (LONG)
  - timing_budget_ms: 100
  - inter_measurement_ms: 100
  - roi_x_size: 16
  - roi_y_size: 16
  - roi_center_spad: 199
  - offset_mm: 0
  - xtalk_cps: 0
  - signal_threshold_kcps: 1024
  - sigma_threshold_mm: 15
  - threshold_low_mm: 0 (disabled)
  - threshold_high_mm: 0 (disabled)
  - threshold_window: 0
  - interrupt_polarity: 1 (active high)
  - i2c_address: 0x29

### Validate Configuration
```c
bool vl53l1x_config_validate(const vl53l1x_config_t *config);
```
- Validates all configuration values are within valid ranges
- Returns `true` if valid, `false` if any value is out of range
- Checks:
  - distance_mode is 1 or 2
  - timing_budget_ms is valid (15, 20, 33, 50, 100, 200, 500)
  - inter_measurement_ms >= timing_budget_ms
  - roi_x_size and roi_y_size are 4-16
  - roi_center_spad is 0-199
  - offset_mm is -128 to +127
  - threshold_high_mm >= threshold_low_mm
  - threshold_window is 0-3
  - interrupt_polarity is 0 or 1
  - i2c_address is valid (0x29-0x7F)

### Apply Configuration
```c
bool vl53l1x_config_apply(vl53l1x_device_handle_t *device, const vl53l1x_config_t *config);
```
- Applies configuration to sensor
- Stops ranging, applies all settings, restarts ranging
- Returns `true` on success, `false` on error
- Should be called after sensor initialization

---

## NVS Storage

**Namespace**: `"vl53l1x"`
**Key**: `"config"`
**Storage Format**: Binary blob (packed structure) for efficient storage
**Size**: ~40 bytes (depending on structure packing)

## Web UI Form Fields

Each configuration parameter should have a corresponding form field:

1. **Distance Mode**: Dropdown/Radio buttons (SHORT/LONG)
2. **Timing Budget**: Dropdown with valid values (15, 20, 33, 50, 100, 200, 500 ms)
3. **Inter-Measurement Period**: Number input (must be >= timing budget)
4. **ROI X Size**: Number input with range 4-16
5. **ROI Y Size**: Number input with range 4-16
6. **ROI Center SPAD**: Number input with range 0-199
7. **Offset**: Number input with range -128 to +127 mm, with "Calibrate" button
8. **Xtalk**: Number input with range 0-65535 cps, with "Calibrate" button
9. **Signal Threshold**: Number input with range 0-65535 kcps
10. **Sigma Threshold**: Number input with range 0-65535 mm
11. **Threshold Low**: Number input with range 0-4000 mm
12. **Threshold High**: Number input with range 0-4000 mm (must be >= low)
13. **Threshold Window**: Dropdown (Below/Above/Out/In)
14. **Interrupt Polarity**: Dropdown/Radio (Active Low/Active High)
15. **I2C Address**: Hex input with range 0x29-0x7F

