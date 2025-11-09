#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "vl53l0x.h"

#define VL53L0X_REG_IDENTIFICATION_MODEL_ID 0xC0
#define VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV 0x89
#define VL53L0X_REG_MSRC_CONFIG_CONTROL 0x60
#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG 0x01
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO 0x0A
#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH 0x84
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR 0x0B
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS 0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS 0x14
#define VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD 0x04
#define VL53L0X_REG_SYSRANGE_START 0x00
#define VL53L0X_REG_VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND 0x30
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0 0xB0
#define VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD 0x4E
#define VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET 0x4F
#define VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT 0xB6
#define VL53L0X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM 0x28
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS 0x8A
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD 0x50
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD 0x70
#define VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP 0x46
#define VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x51
#define VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO 0x52
#define VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x71
#define VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO 0x72
#define VL53L0X_REG_SYSTEM_RANGE_CONFIG 0x09
#define VL53L0X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS 0x20
#define VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44

typedef struct
{
    bool tcc;
    bool msrc;
    bool dss;
    bool pre_range;
    bool final_range;
} sequence_step_enables_t;

typedef struct
{
    uint16_t pre_range_vcsel_period_pclks;
    uint16_t final_range_vcsel_period_pclks;
    uint16_t msrc_dss_tcc_mclks;
    uint16_t pre_range_mclks;
    uint16_t final_range_mclks;
    uint32_t msrc_dss_tcc_us;
    uint32_t pre_range_us;
    uint32_t final_range_us;
} sequence_step_timeouts_t;

static const char *TAG = "vl53l0x";

static esp_err_t write_reg_multi(vl53l0x_t *dev, uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t buffer_len = len + 1;
    uint8_t buffer[32];
    if (buffer_len > sizeof(buffer))
    {
        uint8_t *dynamic_buffer = malloc(buffer_len);
        if (!dynamic_buffer)
        {
            return ESP_ERR_NO_MEM;
        }
        dynamic_buffer[0] = reg;
        memcpy(&dynamic_buffer[1], data, len);
        esp_err_t err = i2c_master_transmit(dev->i2c_dev, dynamic_buffer, buffer_len, pdMS_TO_TICKS(dev->timeout_ms));
        free(dynamic_buffer);
        return err;
    }
    buffer[0] = reg;
    memcpy(&buffer[1], data, len);
    return i2c_master_transmit(dev->i2c_dev, buffer, buffer_len, pdMS_TO_TICKS(dev->timeout_ms));
}

static esp_err_t write_reg8(vl53l0x_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t data = value;
    return write_reg_multi(dev, reg, &data, 1);
}

static esp_err_t write_reg16(vl53l0x_t *dev, uint8_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;
    return write_reg_multi(dev, reg, data, 2);
}

static esp_err_t read_reg_multi(vl53l0x_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, data, len, pdMS_TO_TICKS(dev->timeout_ms));
}

static esp_err_t read_reg8(vl53l0x_t *dev, uint8_t reg, uint8_t *value)
{
    return read_reg_multi(dev, reg, value, 1);
}

static esp_err_t read_reg16(vl53l0x_t *dev, uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    esp_err_t err = read_reg_multi(dev, reg, data, 2);
    if (err != ESP_OK)
    {
        return err;
    }
    *value = ((uint16_t)data[0] << 8) | data[1];
    return ESP_OK;
}

static uint16_t decode_timeout(uint16_t value)
{
    return ((value & 0x00FF) << ((value & 0xFF00) >> 8)) + 1;
}

static uint16_t encode_timeout(uint16_t timeout_mclks)
{
    if (timeout_mclks == 0)
    {
        return 0;
    }
    uint32_t ls_byte = timeout_mclks - 1;
    uint16_t ms_byte = 0;
    while ((ls_byte & 0xFFFFFF00) > 0)
    {
        ls_byte >>= 1;
        ms_byte++;
    }
    return (ms_byte << 8) | (ls_byte & 0xFF);
}

static uint32_t timeout_mclks_to_microseconds(uint16_t timeout_period_mclks, uint32_t vcsel_period_pclks)
{
    uint32_t macro_period_ns = ((uint32_t)2304 * vcsel_period_pclks * 1655 + 500) / 1000;
    return ((timeout_period_mclks * macro_period_ns) + 500) / 1000;
}

static uint16_t timeout_microseconds_to_mclks(uint32_t timeout_us, uint32_t vcsel_period_pclks)
{
    uint32_t macro_period_ns = ((uint32_t)2304 * vcsel_period_pclks * 1655 + 500) / 1000;
    return (uint16_t)(((timeout_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}

static bool get_spad_info(vl53l0x_t *dev, uint8_t *count, bool *is_aperture)
{
    uint8_t tmp = 0;
    if (write_reg8(dev, 0x80, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x00, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x06) != ESP_OK)
    {
        return false;
    }
    if (read_reg8(dev, 0x83, &tmp) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x83, tmp | 0x04) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x07) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x81, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x80, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x94, 0x6B) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x83, 0x00) != ESP_OK)
    {
        return false;
    }
    int64_t start = esp_timer_get_time();
    while (true)
    {
        if (read_reg8(dev, 0x83, &tmp) != ESP_OK)
        {
            return false;
        }
        if (tmp != 0x00)
        {
            break;
        }
        if ((esp_timer_get_time() - start) / 1000 > dev->timeout_ms)
        {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (write_reg8(dev, 0x83, 0x01) != ESP_OK)
    {
        return false;
    }
    if (read_reg8(dev, 0x92, &tmp) != ESP_OK)
    {
        return false;
    }
    *count = tmp & 0x7F;
    *is_aperture = (tmp >> 7) & 0x01;
    if (write_reg8(dev, 0x81, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x06) != ESP_OK)
    {
        return false;
    }
    if (read_reg8(dev, 0x83, &tmp) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x83, tmp & ~0x04) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x00, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x80, 0x00) != ESP_OK)
    {
        return false;
    }
    return true;
}

static void get_sequence_step_enables(vl53l0x_t *dev, sequence_step_enables_t *enables)
{
    uint8_t sequence_config = 0;
    read_reg8(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, &sequence_config);
    enables->tcc = (sequence_config >> 4) & 0x1;
    enables->dss = (sequence_config >> 3) & 0x1;
    enables->msrc = (sequence_config >> 2) & 0x1;
    enables->pre_range = (sequence_config >> 6) & 0x1;
    enables->final_range = (sequence_config >> 7) & 0x1;
}

static void get_sequence_step_timeouts(vl53l0x_t *dev, const sequence_step_enables_t *enables, sequence_step_timeouts_t *timeouts)
{
    timeouts->pre_range_vcsel_period_pclks = 0;
    timeouts->final_range_vcsel_period_pclks = 0;
    timeouts->msrc_dss_tcc_mclks = 0;
    timeouts->pre_range_mclks = 0;
    timeouts->final_range_mclks = 0;
    timeouts->msrc_dss_tcc_us = 0;
    timeouts->pre_range_us = 0;
    timeouts->final_range_us = 0;

    uint8_t reg_val = 0;
    read_reg8(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD, &reg_val);
    timeouts->pre_range_vcsel_period_pclks = ((uint16_t)reg_val + 1) << 1;

    read_reg8(dev, VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP, &reg_val);
    timeouts->msrc_dss_tcc_mclks = reg_val + 1;
    timeouts->msrc_dss_tcc_us = timeout_mclks_to_microseconds(timeouts->msrc_dss_tcc_mclks, timeouts->pre_range_vcsel_period_pclks);

    uint16_t pre_range_timeouts = 0;
    read_reg16(dev, VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, &pre_range_timeouts);
    timeouts->pre_range_mclks = decode_timeout(pre_range_timeouts);
    timeouts->pre_range_us = timeout_mclks_to_microseconds(timeouts->pre_range_mclks, timeouts->pre_range_vcsel_period_pclks);

    read_reg8(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD, &reg_val);
    timeouts->final_range_vcsel_period_pclks = ((uint16_t)reg_val + 1) << 1;

    uint16_t final_range_timeout = 0;
    read_reg16(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, &final_range_timeout);
    if (enables->pre_range)
    {
        final_range_timeout -= encode_timeout(timeouts->pre_range_mclks);
    }
    timeouts->final_range_mclks = decode_timeout(final_range_timeout);
    timeouts->final_range_us = timeout_mclks_to_microseconds(timeouts->final_range_mclks, timeouts->final_range_vcsel_period_pclks);
}

static bool perform_single_ref_calibration(vl53l0x_t *dev, uint8_t vhv_init_byte)
{
    if (write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x01 | vhv_init_byte) != ESP_OK)
    {
        return false;
    }
    int64_t start = esp_timer_get_time();
    while (true)
    {
        uint8_t status = 0;
        if (read_reg8(dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS, &status) != ESP_OK)
        {
            return false;
        }
        if (status & 0x07)
        {
            break;
        }
        if ((esp_timer_get_time() - start) / 1000 > dev->timeout_ms)
        {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x00) != ESP_OK)
    {
        return false;
    }
    return true;
}

static bool set_measurement_timing_budget_internal(vl53l0x_t *dev, uint32_t budget_us)
{
    sequence_step_enables_t enables;
    sequence_step_timeouts_t timeouts;
    get_sequence_step_enables(dev, &enables);
    get_sequence_step_timeouts(dev, &enables, &timeouts);

    uint32_t const overhead_start = 1910;
    uint32_t const overhead_end = 960;
    uint32_t const overhead_tcc = 590;
    uint32_t const overhead_dss = 690;
    uint32_t const overhead_msrc = 660;
    uint32_t const overhead_pre_range = 660;
    uint32_t const overhead_final_range = 550;
    uint32_t const min_timing_budget = 20000;

    if (budget_us < min_timing_budget)
    {
        return false;
    }

    uint32_t used_budget = overhead_start + overhead_end;
    if (enables.tcc)
    {
        used_budget += timeouts.msrc_dss_tcc_us + overhead_tcc;
    }
    if (enables.dss)
    {
        used_budget += 2 * (timeouts.msrc_dss_tcc_us + overhead_dss);
    }
    else if (enables.msrc)
    {
        used_budget += timeouts.msrc_dss_tcc_us + overhead_msrc;
    }
    if (enables.pre_range)
    {
        used_budget += timeouts.pre_range_us + overhead_pre_range;
    }
    if (enables.final_range)
    {
        used_budget += overhead_final_range;
        if (used_budget > budget_us)
        {
            return false;
        }
        uint32_t final_range_timeout_us = budget_us - used_budget;
        uint16_t final_range_timeout_mclks = timeout_microseconds_to_mclks(final_range_timeout_us, timeouts.final_range_vcsel_period_pclks);
        if (enables.pre_range)
        {
            final_range_timeout_mclks += timeouts.pre_range_mclks;
        }
        uint16_t encoded = encode_timeout(final_range_timeout_mclks);
        if (write_reg16(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, encoded) != ESP_OK)
        {
            return false;
        }
        dev->measurement_timing_budget_us = budget_us;
    }
    return true;
}

static uint32_t get_measurement_timing_budget_internal(vl53l0x_t *dev)
{
    sequence_step_enables_t enables;
    sequence_step_timeouts_t timeouts;
    get_sequence_step_enables(dev, &enables);
    get_sequence_step_timeouts(dev, &enables, &timeouts);

    uint32_t budget = 1910 + 960;
    if (enables.tcc)
    {
        budget += timeouts.msrc_dss_tcc_us + 590;
    }
    if (enables.dss)
    {
        budget += 2 * (timeouts.msrc_dss_tcc_us + 690);
    }
    else if (enables.msrc)
    {
        budget += timeouts.msrc_dss_tcc_us + 660;
    }
    if (enables.pre_range)
    {
        budget += timeouts.pre_range_us + 660;
    }
    if (enables.final_range)
    {
        budget += timeouts.final_range_us + 550;
    }
    return budget;
}

static bool load_tuning_settings(vl53l0x_t *dev)
{
    const uint8_t settings[] = {
        0xFF,0x01,0x00,0x00,0xFF,0x00,0x09,0x00,0x10,0x00,0x11,0x00,0x24,0x01,0x25,0xFF,
        0x75,0x00,0xFF,0x01,0x4E,0x2C,0x48,0x00,0x30,0x20,0xFF,0x00,0x30,0x09,0x54,0x00,
        0x31,0x04,0x32,0x03,0x40,0x83,0x46,0x25,0x60,0x00,0x27,0x00,0x50,0x06,0x51,0x00,
        0x52,0x96,0x56,0x08,0x57,0x30,0x61,0x00,0x62,0x00,0x64,0x00,0x65,0x00,0x66,0xA0,
        0xFF,0x01,0x22,0x32,0x47,0x14,0x49,0xFF,0x4A,0x00,0xFF,0x00,0x7A,0x0A,0x7B,0x00,
        0x78,0x21,0xFF,0x01,0x23,0x34,0x42,0x00,0x44,0xFF,0x45,0x26,0x46,0x05,0x40,0x40,
        0x0E,0x06,0x20,0x1A,0x43,0x40,0xFF,0x00,0x34,0x03,0x35,0x44,0xFF,0x01,0x31,0x04,
        0x4B,0x09,0x4C,0x05,0x4D,0x04,0xFF,0x00,0x44,0x00,0x45,0x20,0x47,0x08,0x48,0x28,
        0x67,0x00,0x70,0x04,0x71,0x01,0x72,0xFE,0x76,0x00,0x77,0x00,0xFF,0x01,0x0D,0x01,
        0xFF,0x00,0x80,0x01,0x01,0xF8,0xFF,0x01,0x8E,0x01,0x00,0x01,0xFF,0x00,0x80,0x00
    };
    size_t index = 0;
    while (index < sizeof(settings))
    {
        uint8_t reg = settings[index++];
        uint8_t val = settings[index++];
        if (write_reg8(dev, reg, val) != ESP_OK)
        {
            return false;
        }
    }
    return true;
}

bool vl53l0x_init(vl53l0x_t *dev, const vl53l0x_config_t *cfg)
{
    if (!dev || !cfg || !cfg->i2c_dev)
    {
        return false;
    }
    memset(dev, 0, sizeof(*dev));
    dev->i2c_dev = cfg->i2c_dev;
    dev->timeout_ms = cfg->timeout_ms ? cfg->timeout_ms : 200;

    uint8_t model_id = 0;
    if (read_reg8(dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id) != ESP_OK || model_id != 0xEE)
    {
        ESP_LOGE(TAG, "Unexpected model id 0x%02X", model_id);
        return false;
    }

    uint8_t vhv_config = 0;
    if (read_reg8(dev, VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, &vhv_config) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, vhv_config | 0x01) != ESP_OK)
    {
        return false;
    }

    if (write_reg8(dev, 0x88, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x80, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x00, 0x00) != ESP_OK)
    {
        return false;
    }
    if (read_reg8(dev, 0x91, &dev->stop_variable) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x00, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x80, 0x00) != ESP_OK)
    {
        return false;
    }

    if (write_reg8(dev, VL53L0X_REG_MSRC_CONFIG_CONTROL, 0x12) != ESP_OK)
    {
        return false;
    }
    if (!load_tuning_settings(dev))
    {
        return false;
    }

    if (!set_measurement_timing_budget_internal(dev, 33000))
    {
        return false;
    }

    uint8_t spad_count = 0;
    bool spad_is_aperture = false;
    if (!get_spad_info(dev, &spad_count, &spad_is_aperture))
    {
        return false;
    }
    uint8_t ref_spad_map[6];
    if (read_reg_multi(dev, VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, sizeof(ref_spad_map)) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4) != ESP_OK)
    {
        return false;
    }
    uint8_t spad_enabled_mask[6] = {0};
    uint8_t first_spad = spad_is_aperture ? 12 : 0;
    uint8_t enabled_count = 0;
    for (uint8_t i = 0; i < 48; i++)
    {
        uint8_t mask = 1 << (i % 8);
        if (i < first_spad || enabled_count >= spad_count)
        {
            spad_enabled_mask[i / 8] &= ~mask;
        }
        else
        {
            spad_enabled_mask[i / 8] |= mask;
            enabled_count++;
        }
    }
    if (write_reg_multi(dev, VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spad_enabled_mask, sizeof(spad_enabled_mask)) != ESP_OK)
    {
        return false;
    }

    if (write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04) != ESP_OK)
    {
        return false;
    }
    uint8_t gpio_hv_mux = 0;
    if (read_reg8(dev, VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH, &gpio_hv_mux) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH, gpio_hv_mux & ~0x10) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01) != ESP_OK)
    {
        return false;
    }

    if (!perform_single_ref_calibration(dev, 0x40))
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x01) != ESP_OK)
    {
        return false;
    }
    if (!perform_single_ref_calibration(dev, 0x00))
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8) != ESP_OK)
    {
        return false;
    }

    dev->measurement_timing_budget_us = get_measurement_timing_budget_internal(dev);
    return true;
}

bool vl53l0x_read_range_single_mm(vl53l0x_t *dev, uint16_t *distance_mm, vl53l0x_status_t *status)
{
    if (!distance_mm)
    {
        return false;
    }
    if (status)
    {
        status->data_ready = false;
        status->range_valid = false;
        status->raw_status = 0;
        status->range_status = 0;
        status->out_of_range = false;
    }
    if (write_reg8(dev, 0x80, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x00, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x91, dev->stop_variable) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x00, 0x01) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0xFF, 0x00) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, 0x80, 0x00) != ESP_OK)
    {
        return false;
    }

    if (write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x01) != ESP_OK)
    {
        return false;
    }
    int64_t start = esp_timer_get_time();
    uint8_t irq_status = 0;
    while (true)
    {
        if (read_reg8(dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS, &irq_status) != ESP_OK)
        {
            return false;
        }
        if (irq_status & 0x07)
        {
            break;
        }
        if ((esp_timer_get_time() - start) / 1000 > dev->timeout_ms)
        {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    uint8_t status_code = irq_status & 0x07;
    uint8_t range_status_reg = 0;
    if (read_reg8(dev, VL53L0X_REG_RESULT_RANGE_STATUS, &range_status_reg) != ESP_OK)
    {
        return false;
    }
    uint8_t range_status = (range_status_reg >> 3) & 0x1F;

    if (read_reg16(dev, VL53L0X_REG_RESULT_RANGE_STATUS + 10, distance_mm) != ESP_OK)
    {
        return false;
    }
    if (write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01) != ESP_OK)
    {
        return false;
    }
    if (status)
    {
        status->raw_status = status_code;
        status->data_ready = status_code != 0;
        status->range_status = range_status;
        switch (range_status)
        {
            case 0:
            case 3:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                status->range_valid = true;
                break;
            default:
                status->range_valid = false;
                break;
        }
        status->out_of_range = (*distance_mm >= 8190) || (range_status == 4);
    }
    return true;
}

bool vl53l0x_set_measurement_timing_budget(vl53l0x_t *dev, uint32_t budget_us)
{
    return set_measurement_timing_budget_internal(dev, budget_us);
}

uint32_t vl53l0x_get_measurement_timing_budget(vl53l0x_t *dev)
{
    return dev->measurement_timing_budget_us ? dev->measurement_timing_budget_us : get_measurement_timing_budget_internal(dev);
}

bool vl53l0x_get_status(vl53l0x_t *dev, vl53l0x_status_t *status)
{
    if (!status)
    {
        return false;
    }
    uint8_t irq_status = 0;
    if (read_reg8(dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS, &irq_status) != ESP_OK)
    {
        return false;
    }
    uint8_t range_status_reg = 0;
    if (read_reg8(dev, VL53L0X_REG_RESULT_RANGE_STATUS, &range_status_reg) != ESP_OK)
    {
        return false;
    }
    uint8_t range_status = (range_status_reg >> 3) & 0x1F;
    status->raw_status = irq_status & 0x07;
    status->data_ready = status->raw_status != 0;
    status->range_status = range_status;
    switch (range_status)
    {
        case 0:
        case 3:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            status->range_valid = true;
            break;
        default:
            status->range_valid = false;
            break;
    }
    status->out_of_range = (range_status == 4);
    return true;
}

