#ifndef BNO086_H
#define BNO086_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BNO086_I2C_ADDR 0x4A
#define BNO086_REG_CHANNEL0 0x00

#define BNO086_CHANNEL_COMMAND 0
#define BNO086_CHANNEL_EXECUTABLE 1
#define BNO086_CHANNEL_REPORTS 2

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} bno086_t;

typedef struct {
    float w;
    float x;
    float y;
    float z;
    float accuracy_radians;
    uint8_t status;
} bno086_rotation_vector_t;

typedef struct {
    float roll;
    float pitch;
    float yaw;
} bno086_euler_t;

bool bno086_init(bno086_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t bno086_write(bno086_t *dev, const uint8_t *data, size_t length);
esp_err_t bno086_read(bno086_t *dev, uint8_t *buffer, size_t length);
esp_err_t bno086_reset(bno086_t *dev);
esp_err_t bno086_write_packet(bno086_t *dev, uint8_t channel, const uint8_t *payload, size_t length);
esp_err_t bno086_read_packet(bno086_t *dev, uint8_t *channel, uint8_t *buffer, size_t buffer_length, size_t *payload_length);
esp_err_t bno086_parse_rotation_vector(const uint8_t *payload, size_t length, bno086_rotation_vector_t *rotation);
void bno086_rotation_vector_to_euler(const bno086_rotation_vector_t *rotation, bno086_euler_t *euler);

#ifdef __cplusplus
}
#endif

#endif

