#ifndef MCP4725_H
#define MCP4725_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCP4725_I2C_ADDR_DEFAULT 0x60

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} mcp4725_t;

typedef enum {
    MCP4725_POWER_MODE_NORMAL = 0,
    MCP4725_POWER_MODE_PD_1K = 1,
    MCP4725_POWER_MODE_PD_100K = 2,
    MCP4725_POWER_MODE_PD_500K = 3
} mcp4725_power_mode_t;

typedef struct {
    uint16_t dac_value;
    uint16_t eeprom_value;
    mcp4725_power_mode_t power_mode;
    bool busy;
} mcp4725_status_t;

bool mcp4725_init(mcp4725_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t mcp4725_write_dac(mcp4725_t *dev, uint16_t value);
esp_err_t mcp4725_write_eeprom(mcp4725_t *dev, uint16_t value);
esp_err_t mcp4725_write_dac_mode(mcp4725_t *dev, uint16_t value, mcp4725_power_mode_t mode);
esp_err_t mcp4725_set_power_mode(mcp4725_t *dev, mcp4725_power_mode_t mode);
esp_err_t mcp4725_read_status(mcp4725_t *dev, mcp4725_status_t *status);

#ifdef __cplusplus
}
#endif

#endif

