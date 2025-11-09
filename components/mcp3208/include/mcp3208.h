#ifndef MCP3208_H
#define MCP3208_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    spi_device_handle_t spi_dev;
    int gpio_cs;
} mcp3208_t;

esp_err_t mcp3208_init(mcp3208_t *dev,
                       spi_device_handle_t spi_dev,
                       int gpio_cs);

esp_err_t mcp3208_read_raw(mcp3208_t *dev, uint8_t channel, bool single_ended, uint16_t *value);
esp_err_t mcp3208_read_average(mcp3208_t *dev, uint8_t channel, bool single_ended, uint8_t samples, uint16_t *value);
esp_err_t mcp3208_read_voltage(mcp3208_t *dev, uint8_t channel, bool single_ended, float reference_voltage, float *voltage);

#ifdef __cplusplus
}
#endif

#endif

