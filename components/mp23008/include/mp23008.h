#ifndef MP23008_H
#define MP23008_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MP23008_I2C_ADDR_DEFAULT 0x20

#define MP23008_REG_IODIR   0x00
#define MP23008_REG_IPOL    0x01
#define MP23008_REG_GPINTEN 0x02
#define MP23008_REG_DEFVAL  0x03
#define MP23008_REG_INTCON  0x04
#define MP23008_REG_IOCON   0x05
#define MP23008_REG_GPPU    0x06
#define MP23008_REG_INTF    0x07
#define MP23008_REG_INTCAP  0x08
#define MP23008_REG_GPIO    0x09
#define MP23008_REG_OLAT    0x0A

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} mp23008_t;

typedef struct {
    uint8_t iodir;
    uint8_t ipol;
    uint8_t gpinten;
    uint8_t defval;
    uint8_t intcon;
    uint8_t iocon;
    uint8_t gppu;
} mp23008_config_t;

bool mp23008_init(mp23008_t *dev,
                  i2c_master_dev_handle_t i2c_dev,
                  const mp23008_config_t *cfg);

esp_err_t mp23008_write_register(mp23008_t *dev, uint8_t reg, uint8_t value);
esp_err_t mp23008_read_register(mp23008_t *dev, uint8_t reg, uint8_t *value);

esp_err_t mp23008_write_gpio(mp23008_t *dev, uint8_t value);
esp_err_t mp23008_read_gpio(mp23008_t *dev, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif

