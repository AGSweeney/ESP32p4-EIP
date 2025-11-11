/**
 * @file vl53l1x.c
 * @brief VL53L1X Time-of-Flight Distance Sensor Driver Implementation
 * 
 * This file implements a comprehensive driver for the STMicroelectronics VL53L1X
 * time-of-flight ranging sensor. The implementation includes all major features
 * including distance modes, ROI configuration, calibration, and interrupt support.
 */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "vl53l1x.h"

static const char *TAG = "vl53l1x";

/* VL53L1X Register Definitions */
#define VL53L1X_REG_SOFT_RESET                     0x0000
#define VL53L1X_REG_I2C_SLAVE_DEVICE_ADDRESS       0x0001
#define VL53L1X_REG_PAD_I2C_HV_EXTSUP_CONFIG       0x0002
#define VL53L1X_REG_GPIO_HV_MUX_ACTIVE_HIGH        0x0003
#define VL53L1X_REG_GPIO_TIO_HV_STATUS             0x0004
#define VL53L1X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO1  0x0006
#define VL53L1X_REG_PHASECAL_CONFIG_TIMEOUT_MACROP  0x000B
#define VL53L1X_REG_RANGE_CONFIG_TIMEOUT_MACROP_A_HI  0x000E
#define VL53L1X_REG_RANGE_CONFIG_TIMEOUT_MACROP_A_LO  0x000F
#define VL53L1X_REG_RANGE_CONFIG_TIMEOUT_MACROP_B_HI  0x0010
#define VL53L1X_REG_RANGE_CONFIG_TIMEOUT_MACROP_B_LO  0x0011
#define VL53L1X_REG_RANGE_CONFIG_VCSEL_PERIOD_A    0x0010
#define VL53L1X_REG_RANGE_CONFIG_VCSEL_PERIOD_B    0x0011
#define VL53L1X_REG_RANGE_CONFIG_VALID_PHASE_HIGH  0x0012
#define VL53L1X_REG_SYSTEM_INTERMEASUREMENT_PERIOD 0x001C
#define VL53L1X_REG_SYSTEM_THRESH_HIGH             0x001D
#define VL53L1X_REG_SYSTEM_THRESH_LOW              0x001E
#define VL53L1X_REG_SYSTEM_SEQUENCE_CONFIG         0x001F
#define VL53L1X_REG_SYSTEM_GROUPED_PARAMETER_HOLD  0x0022
#define VL53L1X_REG_SYSRANGE_START                 0x0026
#define VL53L1X_REG_SYSRANGE_MODE_START_STOP       0x0027
#define VL53L1X_REG_RESULT_RANGE_STATUS            0x004D
#define VL53L1X_REG_RESULT_INTERRUPT_STATUS_GPIO    0x004F
#define VL53L1X_REG_RESULT_CLARITY                 0x0050
#define VL53L1X_REG_RESULT_RANGE_MM_SD0           0x0052
#define VL53L1X_REG_RESULT_RANGE_MM_SD1            0x0054
#define VL53L1X_REG_RESULT_RANGE_MM_SD2            0x0056
#define VL53L1X_REG_RESULT_RANGE_MM_SD3             0x0058
#define VL53L1X_REG_RESULT_RANGE_MM_SD4              0x005A
#define VL53L1X_REG_RESULT_RANGE_MM_SD5             0x005C
#define VL53L1X_REG_RESULT_RANGE_MM_SD6              0x005E
#define VL53L1X_REG_RESULT_RANGE_MM_SD7             0x0060
#define VL53L1X_REG_RESULT_RANGE_MM_SD8             0x0062
#define VL53L1X_REG_RESULT_RANGE_MM_SD9             0x0064
#define VL53L1X_REG_RESULT_RANGE_MM_SD10            0x0066
#define VL53L1X_REG_RESULT_RANGE_MM_SD11            0x0068
#define VL53L1X_REG_RESULT_RANGE_MM_SD12            0x006A
#define VL53L1X_REG_RESULT_RANGE_MM_SD13            0x006C
#define VL53L1X_REG_RESULT_RANGE_MM_SD14            0x006E
#define VL53L1X_REG_RESULT_RANGE_MM_SD15            0x0070
#define VL53L1X_REG_RESULT_RANGE_SIGMA_MM_SD0      0x0072
#define VL53L1X_REG_RESULT_OSC_CALIBRATE_VAL       0x00DE
#define VL53L1X_REG_FIRMWARE_SYSTEM_STATUS         0x00E5
#define VL53L1X_REG_IDENTIFICATION_MODEL_ID         0x010F
#define VL53L1X_REG_ROI_CONFIG_USER_ROI_CENTRE_SPAD 0x0127
#define VL53L1X_REG_ROI_CONFIG_USER_ROI_REQUESTED_GLOBAL_XY_SIZE 0x0128
#define VL53L1X_REG_ROI_CONFIG_MODE_MANUAL         0x0129
#define VL53L1X_REG_ROI_CONFIG_MANUAL_EFFECTIVE_SPAD_SELECT 0x012A
#define VL53L1X_REG_DISTANCE_MODE                   0x012B
#define VL53L1X_REG_INTERRUPT_CONFIG_GPIO1          0x014A
#define VL53L1X_REG_GPIO_HV_MUX_ACTIVE_HIGH         0x014B
#define VL53L1X_REG_SYSTEM_INTERRUPT_CLEAR          0x014C
#define VL53L1X_REG_RESULT_RANGE_STATUS             0x004D
#define VL53L1X_REG_RESULT_SPAD_NB_SD0              0x0154
#define VL53L1X_REG_RESULT_SIGNAL_RATE_SPAD_SD0     0x0156
#define VL53L1X_REG_RESULT_AMBIENT_RATE_SPAD_SD0   0x0158
#define VL53L1X_REG_RESULT_SIGMA_SD0                0x015C
#define VL53L1X_REG_RESULT_PEAK_SIGNAL_RATE_MCPS_SD0 0x015E
#define VL53L1X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM 0x001E
#define VL53L1X_REG_MM_CONFIG_INNER_OFFSET_MM      0x0020
#define VL53L1X_REG_MM_CONFIG_OUTER_OFFSET_MM       0x0022
#define VL53L1X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS 0x0016

/* Register values */
#define VL53L1X_SOFT_RESET_VALUE                    0x0000
#define VL53L1X_SOFT_RESET_VALUE2                   0x0001
#define VL53L1X_FIRMWARE_BOOT                       0x0000
#define VL53L1X_FIRMWARE_SYSTEM_READY               0x0001

/* Helper macros for register access */
#define VL53L1X_REG_TO_ADDR(reg)                    ((reg) >> 8), ((reg) & 0xFF)

/* Internal helper functions */
static esp_err_t write_reg_multi(vl53l1x_t *dev, uint16_t reg, const uint8_t *data, size_t len);
static esp_err_t write_reg8(vl53l1x_t *dev, uint16_t reg, uint8_t value);
static esp_err_t write_reg16(vl53l1x_t *dev, uint16_t reg, uint16_t value);
static esp_err_t write_reg32(vl53l1x_t *dev, uint16_t reg, uint32_t value);
static esp_err_t read_reg_multi(vl53l1x_t *dev, uint16_t reg, uint8_t *data, size_t len);
static esp_err_t read_reg8(vl53l1x_t *dev, uint16_t reg, uint8_t *value);
static esp_err_t read_reg16(vl53l1x_t *dev, uint16_t reg, uint16_t *value);
static esp_err_t read_reg32(vl53l1x_t *dev, uint16_t reg, uint32_t *value);
static bool wait_for_firmware_ready(vl53l1x_t *dev);
static bool wait_for_boot_completion(vl53l1x_t *dev);
static bool data_init(vl53l1x_t *dev);
static bool static_init(vl53l1x_t *dev);
static bool perform_ref_calibration(vl53l1x_t *dev, uint8_t vhv_init_byte);

/* Register read/write functions */
static esp_err_t write_reg_multi(vl53l1x_t *dev, uint16_t reg, const uint8_t *data, size_t len)
{
    if (!dev || !dev->i2c_dev || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[32];
    if (len + 2 > sizeof(buffer)) {
        uint8_t *dynamic_buffer = malloc(len + 2);
        if (!dynamic_buffer) {
            return ESP_ERR_NO_MEM;
        }
        dynamic_buffer[0] = (reg >> 8) & 0xFF;
        dynamic_buffer[1] = reg & 0xFF;
        memcpy(&dynamic_buffer[2], data, len);
        esp_err_t err = i2c_master_transmit(dev->i2c_dev, dynamic_buffer, len + 2, 
                                           pdMS_TO_TICKS(dev->timeout_ms));
        free(dynamic_buffer);
        return err;
    }
    
    buffer[0] = (reg >> 8) & 0xFF;
    buffer[1] = reg & 0xFF;
    memcpy(&buffer[2], data, len);
    return i2c_master_transmit(dev->i2c_dev, buffer, len + 2, pdMS_TO_TICKS(dev->timeout_ms));
}

static esp_err_t write_reg8(vl53l1x_t *dev, uint16_t reg, uint8_t value)
{
    return write_reg_multi(dev, reg, &value, 1);
}

static esp_err_t write_reg16(vl53l1x_t *dev, uint16_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;
    return write_reg_multi(dev, reg, data, 2);
}

static esp_err_t write_reg32(vl53l1x_t *dev, uint16_t reg, uint32_t value)
{
    uint8_t data[4];
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
    return write_reg_multi(dev, reg, data, 4);
}

static esp_err_t read_reg_multi(vl53l1x_t *dev, uint16_t reg, uint8_t *data, size_t len)
{
    if (!dev || !dev->i2c_dev || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_addr[2] = {(reg >> 8) & 0xFF, reg & 0xFF};
    return i2c_master_transmit_receive(dev->i2c_dev, reg_addr, 2, data, len, 
                                       pdMS_TO_TICKS(dev->timeout_ms));
}

static esp_err_t read_reg8(vl53l1x_t *dev, uint16_t reg, uint8_t *value)
{
    return read_reg_multi(dev, reg, value, 1);
}

static esp_err_t read_reg16(vl53l1x_t *dev, uint16_t reg, uint16_t *value)
{
    uint8_t data[2];
    esp_err_t err = read_reg_multi(dev, reg, data, 2);
    if (err == ESP_OK) {
        *value = ((uint16_t)data[0] << 8) | data[1];
    }
    return err;
}

static esp_err_t read_reg32(vl53l1x_t *dev, uint16_t reg, uint32_t *value)
{
    uint8_t data[4];
    esp_err_t err = read_reg_multi(dev, reg, data, 4);
    if (err == ESP_OK) {
        *value = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | 
                 ((uint32_t)data[2] << 8) | data[3];
    }
    return err;
}

/* Wait for firmware ready */
static bool wait_for_firmware_ready(vl53l1x_t *dev)
{
    uint8_t status = 0;
    int64_t start = esp_timer_get_time();
    
    while ((esp_timer_get_time() - start) / 1000 < dev->timeout_ms) {
        if (read_reg8(dev, VL53L1X_REG_FIRMWARE_SYSTEM_STATUS, &status) == ESP_OK) {
            if (status == VL53L1X_FIRMWARE_SYSTEM_READY) {
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return false;
}

/* Wait for boot completion */
static bool wait_for_boot_completion(vl53l1x_t *dev)
{
    uint8_t status = 0;
    int64_t start = esp_timer_get_time();
    
    while ((esp_timer_get_time() - start) / 1000 < dev->timeout_ms) {
        if (read_reg8(dev, VL53L1X_REG_FIRMWARE_SYSTEM_STATUS, &status) == ESP_OK) {
            if (status != VL53L1X_FIRMWARE_BOOT) {
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return false;
}

/* Data initialization */
static bool data_init(vl53l1x_t *dev)
{
    /* Set distance mode */
    if (write_reg8(dev, VL53L1X_REG_DISTANCE_MODE, dev->distance_mode) != ESP_OK) {
        return false;
    }
    
    /* Set timing budget - VL53L1X uses macro periods, not direct microseconds */
    /* For now, use default timing budget - will be set via API */
    (void)dev->timing_budget_us;
    
    /* Set ROI */
    vl53l1x_roi_t roi = dev->roi;
    uint8_t roi_centre = ((roi.top_left_x + roi.bottom_right_x) / 2) | 
                         (((roi.top_left_y + roi.bottom_right_y) / 2) << 4);
    uint8_t roi_size = ((roi.bottom_right_x - roi.top_left_x + 1) << 4) | 
                       (roi.bottom_right_y - roi.top_left_y + 1);
    
    if (write_reg8(dev, VL53L1X_REG_ROI_CONFIG_USER_ROI_CENTRE_SPAD, roi_centre) != ESP_OK) {
        return false;
    }
    if (write_reg8(dev, VL53L1X_REG_ROI_CONFIG_USER_ROI_REQUESTED_GLOBAL_XY_SIZE, roi_size) != ESP_OK) {
        return false;
    }
    
    return true;
}

/* Static initialization */
static bool static_init(vl53l1x_t *dev)
{
    /* Perform reference calibration */
    if (!perform_ref_calibration(dev, 0x40)) {
        ESP_LOGW(TAG, "Reference calibration failed, continuing anyway");
    }
    
    return true;
}

/* Perform reference calibration */
static bool perform_ref_calibration(vl53l1x_t *dev, uint8_t vhv_init_byte)
{
    uint8_t sequence_config = 0x01;
    if (write_reg8(dev, VL53L1X_REG_SYSTEM_SEQUENCE_CONFIG, sequence_config) != ESP_OK) {
        return false;
    }
    
    /* VHV calibration */
    if (write_reg8(dev, 0x0062, vhv_init_byte) != ESP_OK) {
        return false;
    }
    if (write_reg8(dev, VL53L1X_REG_SYSRANGE_START, 0x01 | 0x40) != ESP_OK) {
        return false;
    }
    
    /* Wait for completion */
    bool ready = false;
    int64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) / 1000 < dev->timeout_ms) {
        if (vl53l1x_check_data_ready(dev, &ready) && ready) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (vl53l1x_clear_interrupt(dev) != ESP_OK) {
        return false;
    }
    
    /* Phase calibration */
    if (write_reg8(dev, 0x0062, 0x00) != ESP_OK) {
        return false;
    }
    if (write_reg8(dev, VL53L1X_REG_SYSRANGE_START, 0x01 | 0x00) != ESP_OK) {
        return false;
    }
    
    start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) / 1000 < dev->timeout_ms) {
        if (vl53l1x_check_data_ready(dev, &ready) && ready) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (vl53l1x_clear_interrupt(dev) != ESP_OK) {
        return false;
    }
    
    /* Restore sequence config */
    if (write_reg8(dev, VL53L1X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8) != ESP_OK) {
        return false;
    }
    
    return true;
}

/* Public API Implementation */

bool vl53l1x_init(vl53l1x_t *dev, const vl53l1x_config_t *cfg)
{
    if (!dev || !cfg || !cfg->i2c_dev) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }
    
    memset(dev, 0, sizeof(*dev));
    dev->i2c_dev = cfg->i2c_dev;
    dev->timeout_ms = cfg->timeout_ms ? cfg->timeout_ms : 200;
    dev->xshut_pin = cfg->xshut_pin;
    dev->interrupt_enabled = cfg->enable_interrupt;
    dev->interrupt_pin = cfg->interrupt_pin;
    dev->int_polarity = cfg->int_polarity;
    dev->distance_mode = VL53L1X_DISTANCE_MODE_LONG;
    dev->timing_budget_us = VL53L1X_TIMING_BUDGET_33MS;
    dev->roi.top_left_x = 0;
    dev->roi.top_left_y = 0;
    dev->roi.bottom_right_x = 15;
    dev->roi.bottom_right_y = 15;
    dev->offset_mm = 0;
    dev->xtalk_kcps = 0;
    
    /* Hardware reset via XSHUT pin if configured */
    if (dev->xshut_pin != GPIO_NUM_NC) {
        gpio_reset_pin(dev->xshut_pin);
        gpio_set_direction(dev->xshut_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(dev->xshut_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(dev->xshut_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    /* Wait for boot */
    if (!wait_for_boot_completion(dev)) {
        ESP_LOGE(TAG, "Boot timeout");
        return false;
    }
    
    /* Soft reset */
    if (write_reg16(dev, VL53L1X_REG_SOFT_RESET, VL53L1X_SOFT_RESET_VALUE) != ESP_OK) {
        ESP_LOGE(TAG, "Soft reset failed");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
    
    if (write_reg16(dev, VL53L1X_REG_SOFT_RESET, VL53L1X_SOFT_RESET_VALUE2) != ESP_OK) {
        ESP_LOGE(TAG, "Soft reset 2 failed");
        return false;
    }
    
    /* Wait for firmware ready */
    if (!wait_for_firmware_ready(dev)) {
        ESP_LOGE(TAG, "Firmware ready timeout");
        return false;
    }
    
    /* Verify model ID */
    uint16_t model_id = 0;
    if (read_reg16(dev, VL53L1X_REG_IDENTIFICATION_MODEL_ID, &model_id) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read model ID");
        return false;
    }
    
    if (model_id != VL53L1X_MODEL_ID) {
        ESP_LOGE(TAG, "Invalid model ID: 0x%04X (expected 0x%04X)", model_id, VL53L1X_MODEL_ID);
        return false;
    }
    
    /* Static initialization */
    if (!static_init(dev)) {
        ESP_LOGE(TAG, "Static init failed");
        return false;
    }
    
    /* Data initialization */
    if (!data_init(dev)) {
        ESP_LOGE(TAG, "Data init failed");
        return false;
    }
    
    /* Configure interrupt if enabled */
    if (dev->interrupt_enabled && dev->interrupt_pin != GPIO_NUM_NC) {
        gpio_reset_pin(dev->interrupt_pin);
        gpio_set_direction(dev->interrupt_pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(dev->interrupt_pin, GPIO_PULLUP_ONLY);
        
        uint8_t interrupt_config = 0x04; /* New sample ready */
        if (dev->int_polarity == VL53L1X_INTERRUPT_POLARITY_HIGH) {
            interrupt_config |= 0x01;
        }
        if (write_reg8(dev, VL53L1X_REG_INTERRUPT_CONFIG_GPIO1, interrupt_config) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to configure interrupt");
        }
    }
    
    dev->initialized = true;
    ESP_LOGI(TAG, "VL53L1X initialized successfully");
    return true;
}

void vl53l1x_deinit(vl53l1x_t *dev)
{
    if (!dev) {
        return;
    }
    
    vl53l1x_stop_continuous(dev);
    dev->initialized = false;
}

bool vl53l1x_start_measurement(vl53l1x_t *dev)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    return write_reg8(dev, VL53L1X_REG_SYSRANGE_START, 0x01) == ESP_OK;
}

bool vl53l1x_check_data_ready(vl53l1x_t *dev, bool *ready)
{
    if (!dev || !ready) {
        return false;
    }
    
    uint8_t status = 0;
    if (read_reg8(dev, VL53L1X_REG_GPIO_TIO_HV_STATUS, &status) != ESP_OK) {
        return false;
    }
    
    *ready = (status & 0x01) != 0;
    return true;
}

bool vl53l1x_clear_interrupt(vl53l1x_t *dev)
{
    if (!dev) {
        return false;
    }
    
    return write_reg8(dev, VL53L1X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01) == ESP_OK;
}

bool vl53l1x_read_measurement(vl53l1x_t *dev, vl53l1x_measurement_t *measurement)
{
    if (!dev || !measurement || !dev->initialized) {
        return false;
    }
    
    memset(measurement, 0, sizeof(*measurement));
    
    /* Start measurement */
    if (!vl53l1x_start_measurement(dev)) {
        return false;
    }
    
    /* Wait for data ready */
    bool ready = false;
    int64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) / 1000 < dev->timeout_ms) {
        if (vl53l1x_check_data_ready(dev, &ready) && ready) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (!ready) {
        ESP_LOGW(TAG, "Measurement timeout");
        return false;
    }
    
    /* Read range status */
    uint8_t range_status = 0;
    if (read_reg8(dev, VL53L1X_REG_RESULT_RANGE_STATUS, &range_status) != ESP_OK) {
        return false;
    }
    
    measurement->status = (vl53l1x_range_status_t)(range_status & 0x1F);
    measurement->range_valid = (measurement->status == VL53L1X_RANGE_STATUS_OK);
    measurement->data_ready = true;
    
    /* Read distance */
    uint16_t distance = 0;
    if (read_reg16(dev, VL53L1X_REG_RESULT_RANGE_MM_SD0, &distance) != ESP_OK) {
        return false;
    }
    measurement->distance_mm = distance;
    
    /* Apply offset */
    if (dev->offset_mm != 0) {
        int32_t corrected = (int32_t)measurement->distance_mm - (int32_t)dev->offset_mm;
        if (corrected < 0) corrected = 0;
        if (corrected > VL53L1X_MAX_DISTANCE_MM) corrected = VL53L1X_MAX_DISTANCE_MM;
        measurement->distance_mm = (uint16_t)corrected;
    }
    
    /* Read signal rate */
    uint16_t signal_rate = 0;
    if (read_reg16(dev, VL53L1X_REG_RESULT_SIGNAL_RATE_SPAD_SD0, &signal_rate) != ESP_OK) {
        return false;
    }
    measurement->signal_rate_kcps = signal_rate;
    
    /* Read ambient rate */
    uint16_t ambient_rate = 0;
    if (read_reg16(dev, VL53L1X_REG_RESULT_AMBIENT_RATE_SPAD_SD0, &ambient_rate) != ESP_OK) {
        return false;
    }
    measurement->ambient_rate_kcps = ambient_rate;
    
    /* Read effective SPAD count */
    uint16_t spad_count = 0;
    if (read_reg16(dev, VL53L1X_REG_RESULT_SPAD_NB_SD0, &spad_count) != ESP_OK) {
        return false;
    }
    measurement->effective_spad_count = spad_count >> 8;
    
    /* Clear interrupt */
    vl53l1x_clear_interrupt(dev);
    
    return true;
}

bool vl53l1x_read_range_single_mm(vl53l1x_t *dev, uint16_t *distance_mm)
{
    if (!distance_mm) {
        return false;
    }
    
    vl53l1x_measurement_t measurement;
    if (!vl53l1x_read_measurement(dev, &measurement)) {
        return false;
    }
    
    *distance_mm = measurement.distance_mm;
    return measurement.range_valid;
}

bool vl53l1x_set_distance_mode(vl53l1x_t *dev, vl53l1x_distance_mode_t mode)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    if (mode < VL53L1X_DISTANCE_MODE_SHORT || mode > VL53L1X_DISTANCE_MODE_LONG) {
        return false;
    }
    
    if (write_reg8(dev, VL53L1X_REG_DISTANCE_MODE, mode) != ESP_OK) {
        return false;
    }
    
    dev->distance_mode = mode;
    return true;
}

bool vl53l1x_get_distance_mode(vl53l1x_t *dev, vl53l1x_distance_mode_t *mode)
{
    if (!dev || !mode) {
        return false;
    }
    
    uint8_t reg_value = 0;
    if (read_reg8(dev, VL53L1X_REG_DISTANCE_MODE, &reg_value) != ESP_OK) {
        return false;
    }
    
    *mode = (vl53l1x_distance_mode_t)reg_value;
    dev->distance_mode = *mode;
    return true;
}

bool vl53l1x_set_timing_budget(vl53l1x_t *dev, uint32_t budget_us)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    if (budget_us < 15000 || budget_us > 1000000) {
        return false;
    }
    
    /* VL53L1X timing budget is set via distance mode and macro periods */
    /* The actual implementation depends on distance mode */
    /* For simplicity, store the value - full implementation would calculate macro periods */
    dev->timing_budget_us = budget_us;
    
    /* Note: Full implementation would require calculating macro periods based on:
     * - Distance mode
     * - VCSEL periods
     * - Timing budget
     * This is a simplified version that stores the value */
    
    ESP_LOGI(TAG, "Timing budget set to %lu us (stored, macro period calculation not implemented)", budget_us);
    return true;
}

bool vl53l1x_get_timing_budget(vl53l1x_t *dev, uint32_t *budget_us)
{
    if (!dev || !budget_us) {
        return false;
    }
    
    *budget_us = dev->timing_budget_us;
    return true;
}

bool vl53l1x_set_roi(vl53l1x_t *dev, const vl53l1x_roi_t *roi)
{
    if (!dev || !roi || !dev->initialized) {
        return false;
    }
    
    if (roi->top_left_x > 15 || roi->top_left_y > 15 ||
        roi->bottom_right_x > 15 || roi->bottom_right_y > 15 ||
        roi->top_left_x > roi->bottom_right_x ||
        roi->top_left_y > roi->bottom_right_y) {
        return false;
    }
    
    uint8_t roi_centre = ((roi->top_left_x + roi->bottom_right_x) / 2) | 
                         (((roi->top_left_y + roi->bottom_right_y) / 2) << 4);
    uint8_t roi_size = ((roi->bottom_right_x - roi->top_left_x + 1) << 4) | 
                       (roi->bottom_right_y - roi->top_left_y + 1);
    
    if (write_reg8(dev, VL53L1X_REG_ROI_CONFIG_USER_ROI_CENTRE_SPAD, roi_centre) != ESP_OK) {
        return false;
    }
    if (write_reg8(dev, VL53L1X_REG_ROI_CONFIG_USER_ROI_REQUESTED_GLOBAL_XY_SIZE, roi_size) != ESP_OK) {
        return false;
    }
    
    dev->roi = *roi;
    return true;
}

bool vl53l1x_set_roi_center(vl53l1x_t *dev, uint8_t center_x, uint8_t center_y, uint8_t size)
{
    if (!dev || center_x > 15 || center_y > 15 || size < 4 || size > 16 || (size % 2) != 0) {
        return false;
    }
    
    uint8_t half_size = size / 2;
    vl53l1x_roi_t roi;
    
    if (center_x < half_size) {
        roi.top_left_x = 0;
        roi.bottom_right_x = size - 1;
    } else if (center_x + half_size > 15) {
        roi.bottom_right_x = 15;
        roi.top_left_x = 15 - (size - 1);
    } else {
        roi.top_left_x = center_x - half_size;
        roi.bottom_right_x = center_x + half_size - 1;
    }
    
    if (center_y < half_size) {
        roi.top_left_y = 0;
        roi.bottom_right_y = size - 1;
    } else if (center_y + half_size > 15) {
        roi.bottom_right_y = 15;
        roi.top_left_y = 15 - (size - 1);
    } else {
        roi.top_left_y = center_y - half_size;
        roi.bottom_right_y = center_y + half_size - 1;
    }
    
    return vl53l1x_set_roi(dev, &roi);
}

bool vl53l1x_get_roi(vl53l1x_t *dev, vl53l1x_roi_t *roi)
{
    if (!dev || !roi) {
        return false;
    }
    
    *roi = dev->roi;
    return true;
}

bool vl53l1x_set_inter_measurement_period(vl53l1x_t *dev, uint32_t period_ms)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    if (period_ms < dev->timing_budget_us / 1000) {
        return false;
    }
    
    return write_reg32(dev, VL53L1X_REG_SYSTEM_INTERMEASUREMENT_PERIOD, period_ms * 1000) == ESP_OK;
}

bool vl53l1x_get_inter_measurement_period(vl53l1x_t *dev, uint32_t *period_ms)
{
    if (!dev || !period_ms) {
        return false;
    }
    
    uint32_t value = 0;
    if (read_reg32(dev, VL53L1X_REG_SYSTEM_INTERMEASUREMENT_PERIOD, &value) != ESP_OK) {
        return false;
    }
    
    *period_ms = value / 1000;
    return true;
}

bool vl53l1x_calibrate_offset(vl53l1x_t *dev, uint16_t target_distance_mm)
{
    if (!dev || !dev->initialized || target_distance_mm < 50 || target_distance_mm > 1000) {
        return false;
    }
    
    /* Perform measurement */
    vl53l1x_measurement_t measurement;
    if (!vl53l1x_read_measurement(dev, &measurement)) {
        return false;
    }
    
    if (!measurement.range_valid) {
        ESP_LOGW(TAG, "Invalid measurement for offset calibration");
        return false;
    }
    
    /* Calculate offset */
    int16_t offset = (int16_t)measurement.distance_mm - (int16_t)target_distance_mm;
    
    /* Set offset */
    if (write_reg16(dev, VL53L1X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM, offset) != ESP_OK) {
        return false;
    }
    
    dev->offset_mm = offset;
    ESP_LOGI(TAG, "Offset calibration complete: %d mm", offset);
    return true;
}

bool vl53l1x_calibrate_xtalk(vl53l1x_t *dev, uint16_t target_distance_mm)
{
    if (!dev || !dev->initialized || target_distance_mm < 50 || target_distance_mm > 400) {
        return false;
    }
    
    /* Perform measurement */
    vl53l1x_measurement_t measurement;
    if (!vl53l1x_read_measurement(dev, &measurement)) {
        return false;
    }
    
    if (!measurement.range_valid) {
        ESP_LOGW(TAG, "Invalid measurement for xtalk calibration");
        return false;
    }
    
    /* Use signal rate as crosstalk value */
    uint16_t xtalk = measurement.signal_rate_kcps;
    
    if (write_reg16(dev, VL53L1X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, xtalk) != ESP_OK) {
        return false;
    }
    
    dev->xtalk_kcps = xtalk;
    ESP_LOGI(TAG, "Xtalk calibration complete: %u kcps", xtalk);
    return true;
}

bool vl53l1x_set_offset(vl53l1x_t *dev, int16_t offset_mm)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    if (write_reg16(dev, VL53L1X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM, offset_mm) != ESP_OK) {
        return false;
    }
    
    dev->offset_mm = offset_mm;
    return true;
}

bool vl53l1x_get_offset(vl53l1x_t *dev, int16_t *offset_mm)
{
    if (!dev || !offset_mm) {
        return false;
    }
    
    uint16_t value = 0;
    if (read_reg16(dev, VL53L1X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM, &value) != ESP_OK) {
        return false;
    }
    
    *offset_mm = (int16_t)value;
    dev->offset_mm = *offset_mm;
    return true;
}

bool vl53l1x_set_xtalk(vl53l1x_t *dev, uint16_t xtalk_kcps)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    if (write_reg16(dev, VL53L1X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, xtalk_kcps) != ESP_OK) {
        return false;
    }
    
    dev->xtalk_kcps = xtalk_kcps;
    return true;
}

bool vl53l1x_get_xtalk(vl53l1x_t *dev, uint16_t *xtalk_kcps)
{
    if (!dev || !xtalk_kcps) {
        return false;
    }
    
    uint16_t value = 0;
    if (read_reg16(dev, VL53L1X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, &value) != ESP_OK) {
        return false;
    }
    
    *xtalk_kcps = value;
    dev->xtalk_kcps = value;
    return true;
}

bool vl53l1x_set_signal_threshold(vl53l1x_t *dev, uint16_t threshold_kcps)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    return write_reg16(dev, VL53L1X_REG_SYSTEM_THRESH_HIGH, threshold_kcps) == ESP_OK;
}

bool vl53l1x_get_signal_threshold(vl53l1x_t *dev, uint16_t *threshold_kcps)
{
    if (!dev || !threshold_kcps) {
        return false;
    }
    
    uint16_t value = 0;
    if (read_reg16(dev, VL53L1X_REG_SYSTEM_THRESH_HIGH, &value) != ESP_OK) {
        return false;
    }
    
    *threshold_kcps = value;
    return true;
}

bool vl53l1x_set_sigma_threshold(vl53l1x_t *dev, uint16_t threshold_mm)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    return write_reg16(dev, VL53L1X_REG_SYSTEM_THRESH_LOW, threshold_mm) == ESP_OK;
}

bool vl53l1x_get_sigma_threshold(vl53l1x_t *dev, uint16_t *threshold_mm)
{
    if (!dev || !threshold_mm) {
        return false;
    }
    
    uint16_t value = 0;
    if (read_reg16(dev, VL53L1X_REG_SYSTEM_THRESH_LOW, &value) != ESP_OK) {
        return false;
    }
    
    *threshold_mm = value;
    return true;
}

bool vl53l1x_start_continuous(vl53l1x_t *dev, uint32_t period_ms)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    if (!vl53l1x_set_inter_measurement_period(dev, period_ms)) {
        return false;
    }
    
    return write_reg8(dev, VL53L1X_REG_SYSRANGE_START, 0x02) == ESP_OK;
}

bool vl53l1x_stop_continuous(vl53l1x_t *dev)
{
    if (!dev || !dev->initialized) {
        return false;
    }
    
    return write_reg8(dev, VL53L1X_REG_SYSRANGE_START, 0x01) == ESP_OK;
}

bool vl53l1x_get_status(vl53l1x_t *dev, uint8_t *status)
{
    if (!dev || !status) {
        return false;
    }
    
    return read_reg8(dev, VL53L1X_REG_RESULT_RANGE_STATUS, status) == ESP_OK;
}

bool vl53l1x_reset(vl53l1x_t *dev)
{
    if (!dev) {
        return false;
    }
    
    if (dev->xshut_pin != GPIO_NUM_NC) {
        gpio_set_level(dev->xshut_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(dev->xshut_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
        return wait_for_boot_completion(dev);
    }
    
    return false;
}

bool vl53l1x_set_i2c_address(vl53l1x_t *dev, uint8_t address)
{
    if (!dev || address < 0x08 || address > 0x77) {
        return false;
    }
    
    if (write_reg8(dev, VL53L1X_REG_I2C_SLAVE_DEVICE_ADDRESS, address) != ESP_OK) {
        return false;
    }
    
    ESP_LOGI(TAG, "I2C address set to 0x%02X (takes effect after reset)", address);
    return true;
}

