/**
 * @file vl53l1x.h
 * @brief VL53L1X Time-of-Flight Distance Sensor Driver for ESP-IDF
 * 
 * This driver provides a comprehensive interface to the STMicroelectronics VL53L1X
 * time-of-flight ranging sensor. The VL53L1X offers improved performance over the
 * VL53L0X with longer range (up to 4m), multiple distance modes, ROI configuration,
 * and advanced calibration features.
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef VL53L1X_H
#define VL53L1X_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup VL53L1X_Constants Constants
 * @{
 */

/** Default I2C address for VL53L1X */
#define VL53L1X_I2C_ADDRESS_DEFAULT         0x29

/** Model ID register value */
#define VL53L1X_MODEL_ID                    0xEACC

/** Maximum distance in mm */
#define VL53L1X_MAX_DISTANCE_MM              4000

/** Minimum distance in mm */
#define VL53L1X_MIN_DISTANCE_MM              0

/** Maximum ROI size */
#define VL53L1X_MAX_ROI_SIZE                 16

/** Minimum ROI size */
#define VL53L1X_MIN_ROI_SIZE                 4

/** @} */

/** @defgroup VL53L1X_DistanceModes Distance Modes
 * @{
 */

/**
 * @brief Distance measurement modes
 * 
 * The VL53L1X supports different distance modes optimized for different
 * use cases. Short mode provides faster measurements with lower range,
 * while long mode provides maximum range with longer measurement times.
 */
typedef enum {
    VL53L1X_DISTANCE_MODE_SHORT = 1,      /**< Short distance mode (up to 1.3m), fastest */
    VL53L1X_DISTANCE_MODE_MEDIUM = 2,    /**< Medium distance mode (up to 3m), balanced */
    VL53L1X_DISTANCE_MODE_LONG = 3       /**< Long distance mode (up to 4m), slowest but most accurate */
} vl53l1x_distance_mode_t;

/** @} */

/** @defgroup VL53L1X_TimingBudget Timing Budgets
 * @{
 */

/**
 * @brief Predefined timing budgets in microseconds
 * 
 * Timing budget determines the measurement time. Longer budgets provide
 * better accuracy but slower measurements. The sensor supports budgets
 * from 15ms to 1000ms.
 */
typedef enum {
    VL53L1X_TIMING_BUDGET_15MS = 15000,      /**< 15ms - Fastest, lowest accuracy */
    VL53L1X_TIMING_BUDGET_20MS = 20000,      /**< 20ms - Fast */
    VL53L1X_TIMING_BUDGET_33MS = 33000,      /**< 33ms - Default */
    VL53L1X_TIMING_BUDGET_50MS = 50000,      /**< 50ms - Balanced */
    VL53L1X_TIMING_BUDGET_100MS = 100000,    /**< 100ms - Good accuracy */
    VL53L1X_TIMING_BUDGET_200MS = 200000,    /**< 200ms - High accuracy */
    VL53L1X_TIMING_BUDGET_500MS = 500000,    /**< 500ms - Very high accuracy */
    VL53L1X_TIMING_BUDGET_1000MS = 1000000   /**< 1000ms - Maximum accuracy */
} vl53l1x_timing_budget_t;

/** @} */

/** @defgroup VL53L1X_InterruptPolarity Interrupt Polarity
 * @{
 */

/**
 * @brief Interrupt pin polarity
 */
typedef enum {
    VL53L1X_INTERRUPT_POLARITY_LOW = 0,   /**< Active low interrupt */
    VL53L1X_INTERRUPT_POLARITY_HIGH = 1   /**< Active high interrupt */
} vl53l1x_interrupt_polarity_t;

/** @} */

/** @defgroup VL53L1X_RangeStatus Range Status Codes
 * @{
 */

/**
 * @brief Range measurement status codes
 */
typedef enum {
    VL53L1X_RANGE_STATUS_OK = 0,                    /**< Valid range measurement */
    VL53L1X_RANGE_STATUS_SIGMA_FAIL = 1,           /**< Sigma estimator check failed */
    VL53L1X_RANGE_STATUS_SIGNAL_FAIL = 2,          /**< Signal strength check failed */
    VL53L1X_RANGE_STATUS_OUT_OF_BOUNDS_FAIL = 3,   /**< Target out of bounds */
    VL53L1X_RANGE_STATUS_HARDWARE_FAIL = 4,        /**< Hardware failure */
    VL53L1X_RANGE_STATUS_RANGE_VALID_MIN = 5,      /**< Range valid but below minimum */
    VL53L1X_RANGE_STATUS_RANGE_VALID_MAX = 6,      /**< Range valid but above maximum */
    VL53L1X_RANGE_STATUS_WRAP_TARGET_FAIL = 7,     /**< Wrapped target fail */
    VL53L1X_RANGE_STATUS_PROCESSING_FAIL = 8,      /**< Processing failure */
    VL53L1X_RANGE_STATUS_XTALK_SIGNAL_FAIL = 9,    /**< Xtalk signal check failed */
    VL53L1X_RANGE_STATUS_UPDATE_FAIL = 10,         /**< Update failure */
    VL53L1X_RANGE_STATUS_WRAP_TARGET_FAIL2 = 11,   /**< Wrapped target fail 2 */
    VL53L1X_RANGE_STATUS_NO_UPDATE = 255           /**< No update */
} vl53l1x_range_status_t;

/** @} */

/** @defgroup VL53L1X_CalibrationType Calibration Types
 * @{
 */

/**
 * @brief Calibration types
 */
typedef enum {
    VL53L1X_CALIBRATION_OFFSET = 0,        /**< Offset calibration */
    VL53L1X_CALIBRATION_XTALK = 1         /**< Crosstalk calibration */
} vl53l1x_calibration_type_t;

/** @} */

/** @defgroup VL53L1X_Structs Structures
 * @{
 */

/**
 * @brief ROI (Region of Interest) configuration
 */
typedef struct {
    uint8_t top_left_x;      /**< Top-left X coordinate (0-15) */
    uint8_t top_left_y;      /**< Top-left Y coordinate (0-15) */
    uint8_t bottom_right_x;  /**< Bottom-right X coordinate (0-15) */
    uint8_t bottom_right_y;  /**< Bottom-right Y coordinate (0-15) */
} vl53l1x_roi_t;

/**
 * @brief Measurement result structure
 */
typedef struct {
    uint16_t distance_mm;              /**< Measured distance in millimeters */
    uint16_t signal_rate_kcps;         /**< Signal rate in kcps (kilo counts per second) */
    uint16_t ambient_rate_kcps;        /**< Ambient rate in kcps */
    uint8_t effective_spad_count;     /**< Effective SPAD count */
    vl53l1x_range_status_t status;   /**< Range status code */
    bool range_valid;                  /**< True if range measurement is valid */
    bool data_ready;                    /**< True if new data is ready */
} vl53l1x_measurement_t;

/**
 * @brief Device configuration structure
 */
typedef struct {
    i2c_master_dev_handle_t i2c_dev;           /**< I2C device handle */
    uint16_t timeout_ms;                       /**< Operation timeout in milliseconds */
    gpio_num_t xshut_pin;                      /**< XSHUT pin (GPIO_NUM_NC if not used) */
    bool enable_interrupt;                     /**< Enable interrupt pin */
    gpio_num_t interrupt_pin;                  /**< Interrupt GPIO pin (if enabled) */
    vl53l1x_interrupt_polarity_t int_polarity; /**< Interrupt polarity */
} vl53l1x_config_t;

/**
 * @brief Device handle structure
 */
typedef struct {
    i2c_master_dev_handle_t i2c_dev;
    uint16_t timeout_ms;
    vl53l1x_distance_mode_t distance_mode;
    uint32_t timing_budget_us;
    vl53l1x_roi_t roi;
    bool interrupt_enabled;
    gpio_num_t interrupt_pin;
    vl53l1x_interrupt_polarity_t int_polarity;
    gpio_num_t xshut_pin;
    bool initialized;
    uint16_t offset_mm;
    uint16_t xtalk_kcps;
} vl53l1x_t;

/** @} */

/** @defgroup VL53L1X_Functions Functions
 * @{
 */

/**
 * @brief Initialize the VL53L1X sensor
 * 
 * This function performs the full initialization sequence including:
 * - Hardware reset (if XSHUT pin configured)
 * - Model ID verification
 * - Firmware loading
 * - Calibration data loading
 * - Default configuration
 * 
 * @param dev Pointer to device handle structure
 * @param cfg Pointer to configuration structure
 * @return true on success, false on failure
 */
bool vl53l1x_init(vl53l1x_t *dev, const vl53l1x_config_t *cfg);

/**
 * @brief Deinitialize the VL53L1X sensor
 * 
 * @param dev Pointer to device handle structure
 */
void vl53l1x_deinit(vl53l1x_t *dev);

/**
 * @brief Start a single ranging measurement
 * 
 * @param dev Pointer to device handle structure
 * @return true on success, false on failure
 */
bool vl53l1x_start_measurement(vl53l1x_t *dev);

/**
 * @brief Check if measurement data is ready
 * 
 * @param dev Pointer to device handle structure
 * @param ready Pointer to store ready status
 * @return true on success, false on failure
 */
bool vl53l1x_check_data_ready(vl53l1x_t *dev, bool *ready);

/**
 * @brief Clear the interrupt flag
 * 
 * @param dev Pointer to device handle structure
 * @return true on success, false on failure
 */
bool vl53l1x_clear_interrupt(vl53l1x_t *dev);

/**
 * @brief Read a single distance measurement (blocking)
 * 
 * This function starts a measurement, waits for completion, and reads the result.
 * 
 * @param dev Pointer to device handle structure
 * @param measurement Pointer to measurement result structure
 * @return true on success, false on failure
 */
bool vl53l1x_read_measurement(vl53l1x_t *dev, vl53l1x_measurement_t *measurement);

/**
 * @brief Read a single distance measurement in millimeters (blocking)
 * 
 * Simplified function that returns just the distance value.
 * 
 * @param dev Pointer to device handle structure
 * @param distance_mm Pointer to store distance in millimeters
 * @return true on success, false on failure
 */
bool vl53l1x_read_range_single_mm(vl53l1x_t *dev, uint16_t *distance_mm);

/**
 * @brief Set the distance mode
 * 
 * @param dev Pointer to device handle structure
 * @param mode Distance mode (SHORT, MEDIUM, or LONG)
 * @return true on success, false on failure
 */
bool vl53l1x_set_distance_mode(vl53l1x_t *dev, vl53l1x_distance_mode_t mode);

/**
 * @brief Get the current distance mode
 * 
 * @param dev Pointer to device handle structure
 * @param mode Pointer to store distance mode
 * @return true on success, false on failure
 */
bool vl53l1x_get_distance_mode(vl53l1x_t *dev, vl53l1x_distance_mode_t *mode);

/**
 * @brief Set the measurement timing budget
 * 
 * @param dev Pointer to device handle structure
 * @param budget_us Timing budget in microseconds (15000-1000000)
 * @return true on success, false on failure
 */
bool vl53l1x_set_timing_budget(vl53l1x_t *dev, uint32_t budget_us);

/**
 * @brief Get the current measurement timing budget
 * 
 * @param dev Pointer to device handle structure
 * @param budget_us Pointer to store timing budget in microseconds
 * @return true on success, false on failure
 */
bool vl53l1x_get_timing_budget(vl53l1x_t *dev, uint32_t *budget_us);

/**
 * @brief Set the Region of Interest (ROI)
 * 
 * The ROI defines the area of the sensor's field of view to use for ranging.
 * The sensor has a 16x16 SPAD array. Coordinates range from 0-15.
 * 
 * @param dev Pointer to device handle structure
 * @param roi Pointer to ROI configuration structure
 * @return true on success, false on failure
 */
bool vl53l1x_set_roi(vl53l1x_t *dev, const vl53l1x_roi_t *roi);

/**
 * @brief Set ROI using center and size
 * 
 * Convenience function to set ROI using center point and size.
 * 
 * @param dev Pointer to device handle structure
 * @param center_x Center X coordinate (0-15)
 * @param center_y Center Y coordinate (0-15)
 * @param size ROI size (4-16, must be even)
 * @return true on success, false on failure
 */
bool vl53l1x_set_roi_center(vl53l1x_t *dev, uint8_t center_x, uint8_t center_y, uint8_t size);

/**
 * @brief Get the current ROI configuration
 * 
 * @param dev Pointer to device handle structure
 * @param roi Pointer to store ROI configuration
 * @return true on success, false on failure
 */
bool vl53l1x_get_roi(vl53l1x_t *dev, vl53l1x_roi_t *roi);

/**
 * @brief Set the inter-measurement period
 * 
 * This is the time between consecutive measurements when in continuous mode.
 * Must be >= timing budget.
 * 
 * @param dev Pointer to device handle structure
 * @param period_ms Inter-measurement period in milliseconds
 * @return true on success, false on failure
 */
bool vl53l1x_set_inter_measurement_period(vl53l1x_t *dev, uint32_t period_ms);

/**
 * @brief Get the inter-measurement period
 * 
 * @param dev Pointer to device handle structure
 * @param period_ms Pointer to store period in milliseconds
 * @return true on success, false on failure
 */
bool vl53l1x_get_inter_measurement_period(vl53l1x_t *dev, uint32_t *period_ms);

/**
 * @brief Perform offset calibration
 * 
 * Calibrates the sensor for a specific target distance. The target should
 * be a flat, reflective surface at the specified distance.
 * 
 * @param dev Pointer to device handle structure
 * @param target_distance_mm Target distance in millimeters (typically 100-400mm)
 * @return true on success, false on failure
 */
bool vl53l1x_calibrate_offset(vl53l1x_t *dev, uint16_t target_distance_mm);

/**
 * @brief Perform crosstalk calibration
 * 
 * Calibrates the sensor to compensate for optical crosstalk. The target
 * should be a flat, reflective surface at close range (typically 100mm).
 * 
 * @param dev Pointer to device handle structure
 * @param target_distance_mm Target distance in millimeters (typically 100mm)
 * @return true on success, false on failure
 */
bool vl53l1x_calibrate_xtalk(vl53l1x_t *dev, uint16_t target_distance_mm);

/**
 * @brief Set manual offset value
 * 
 * @param dev Pointer to device handle structure
 * @param offset_mm Offset in millimeters
 * @return true on success, false on failure
 */
bool vl53l1x_set_offset(vl53l1x_t *dev, int16_t offset_mm);

/**
 * @brief Get the current offset value
 * 
 * @param dev Pointer to device handle structure
 * @param offset_mm Pointer to store offset in millimeters
 * @return true on success, false on failure
 */
bool vl53l1x_get_offset(vl53l1x_t *dev, int16_t *offset_mm);

/**
 * @brief Set manual crosstalk value
 * 
 * @param dev Pointer to device handle structure
 * @param xtalk_kcps Crosstalk in kcps (kilo counts per second)
 * @return true on success, false on failure
 */
bool vl53l1x_set_xtalk(vl53l1x_t *dev, uint16_t xtalk_kcps);

/**
 * @brief Get the current crosstalk value
 * 
 * @param dev Pointer to device handle structure
 * @param xtalk_kcps Pointer to store crosstalk in kcps
 * @return true on success, false on failure
 */
bool vl53l1x_get_xtalk(vl53l1x_t *dev, uint16_t *xtalk_kcps);

/**
 * @brief Set the signal threshold for range valid detection
 * 
 * @param dev Pointer to device handle structure
 * @param threshold_kcps Signal threshold in kcps
 * @return true on success, false on failure
 */
bool vl53l1x_set_signal_threshold(vl53l1x_t *dev, uint16_t threshold_kcps);

/**
 * @brief Get the signal threshold
 * 
 * @param dev Pointer to device handle structure
 * @param threshold_kcps Pointer to store threshold in kcps
 * @return true on success, false on failure
 */
bool vl53l1x_get_signal_threshold(vl53l1x_t *dev, uint16_t *threshold_kcps);

/**
 * @brief Set the sigma threshold for range valid detection
 * 
 * @param dev Pointer to device handle structure
 * @param threshold_mm Sigma threshold in millimeters
 * @return true on success, false on failure
 */
bool vl53l1x_set_sigma_threshold(vl53l1x_t *dev, uint16_t threshold_mm);

/**
 * @brief Get the sigma threshold
 * 
 * @param dev Pointer to device handle structure
 * @param threshold_mm Pointer to store threshold in millimeters
 * @return true on success, false on failure
 */
bool vl53l1x_get_sigma_threshold(vl53l1x_t *dev, uint16_t *threshold_mm);

/**
 * @brief Start continuous ranging measurements
 * 
 * @param dev Pointer to device handle structure
 * @param period_ms Inter-measurement period in milliseconds
 * @return true on success, false on failure
 */
bool vl53l1x_start_continuous(vl53l1x_t *dev, uint32_t period_ms);

/**
 * @brief Stop continuous ranging measurements
 * 
 * @param dev Pointer to device handle structure
 * @return true on success, false on failure
 */
bool vl53l1x_stop_continuous(vl53l1x_t *dev);

/**
 * @brief Get device status
 * 
 * @param dev Pointer to device handle structure
 * @param status Pointer to store status byte
 * @return true on success, false on failure
 */
bool vl53l1x_get_status(vl53l1x_t *dev, uint8_t *status);

/**
 * @brief Reset the sensor via XSHUT pin
 * 
 * @param dev Pointer to device handle structure
 * @return true on success, false on failure
 */
bool vl53l1x_reset(vl53l1x_t *dev);

/**
 * @brief Set the I2C address
 * 
 * This allows multiple VL53L1X sensors on the same I2C bus.
 * The address change takes effect after a power cycle or reset.
 * 
 * @param dev Pointer to device handle structure
 * @param address New I2C address (0x08-0x77)
 * @return true on success, false on failure
 */
bool vl53l1x_set_i2c_address(vl53l1x_t *dev, uint8_t address);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* VL53L1X_H */

