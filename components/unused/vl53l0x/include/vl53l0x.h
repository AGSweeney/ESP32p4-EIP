#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"

#define VL53L0X_I2C_ADDRESS_DEFAULT 0x29

typedef struct
{
    i2c_master_dev_handle_t i2c_dev;
    uint16_t timeout_ms;
} vl53l0x_config_t;

typedef struct
{
    i2c_master_dev_handle_t i2c_dev;
    uint16_t timeout_ms;
    uint32_t measurement_timing_budget_us;
    uint8_t stop_variable;
} vl53l0x_t;

typedef struct
{
    bool data_ready;
    bool range_valid;
    uint8_t raw_status;
    uint8_t range_status;
    bool out_of_range;
} vl53l0x_status_t;

bool vl53l0x_init(vl53l0x_t *dev, const vl53l0x_config_t *cfg);
bool vl53l0x_read_range_single_mm(vl53l0x_t *dev, uint16_t *distance_mm, vl53l0x_status_t *status);
bool vl53l0x_set_measurement_timing_budget(vl53l0x_t *dev, uint32_t budget_us);
uint32_t vl53l0x_get_measurement_timing_budget(vl53l0x_t *dev);
bool vl53l0x_get_status(vl53l0x_t *dev, vl53l0x_status_t *status);

#endif

