#include "bno086.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

bool bno086_init(bno086_t *dev, i2c_master_dev_handle_t i2c_dev)
{
    if (!dev || !i2c_dev)
    {
        return false;
    }
    dev->i2c_dev = i2c_dev;
    return true;
}

esp_err_t bno086_write(bno086_t *dev, const uint8_t *data, size_t length)
{
    if (!dev || !data || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit(dev->i2c_dev, data, length, pdMS_TO_TICKS(100));
}

esp_err_t bno086_read(bno086_t *dev, uint8_t *buffer, size_t length)
{
    if (!dev || !buffer || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_receive(dev->i2c_dev, buffer, length, pdMS_TO_TICKS(100));
}

esp_err_t bno086_reset(bno086_t *dev)
{
    if (!dev)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t payload[2] = {0x3F, 0x01};
    return i2c_master_transmit(dev->i2c_dev, payload, sizeof(payload), pdMS_TO_TICKS(100));
}

esp_err_t bno086_write_packet(bno086_t *dev, uint8_t channel, const uint8_t *payload, size_t length)
{
    if (!dev || (!payload && length > 0))
    {
        return ESP_ERR_INVALID_ARG;
    }
    size_t frame_len = length + 4;
    uint8_t *frame = (uint8_t *)malloc(frame_len);
    if (!frame)
    {
        return ESP_ERR_NO_MEM;
    }
    frame[0] = BNO086_REG_CHANNEL0;
    frame[1] = (uint8_t)(length & 0xFF);
    frame[2] = (uint8_t)((length >> 8) & 0xFF);
    frame[3] = channel;
    if (length > 0)
    {
        memcpy(&frame[4], payload, length);
    }
    esp_err_t err = i2c_master_transmit(dev->i2c_dev, frame, frame_len, pdMS_TO_TICKS(100));
    free(frame);
    return err;
}

esp_err_t bno086_read_packet(bno086_t *dev, uint8_t *channel, uint8_t *buffer, size_t buffer_length, size_t *payload_length)
{
    if (!dev || !buffer || buffer_length < 4)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = BNO086_REG_CHANNEL0;
    uint8_t header[3] = {0};
    esp_err_t err = i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, header, sizeof(header), pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }
    uint16_t length = (uint16_t)(header[0] | (header[1] << 8));
    if (payload_length)
    {
        *payload_length = length;
    }
    if (length == 0)
    {
        if (channel)
        {
            *channel = header[2];
        }
        return ESP_OK;
    }
    if (length > buffer_length)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    err = i2c_master_receive(dev->i2c_dev, buffer, length, pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }
    if (channel)
    {
        *channel = header[2];
    }
    return ESP_OK;
}

static float clampf(float value, float min_val, float max_val)
{
    if (value < min_val)
    {
        return min_val;
    }
    if (value > max_val)
    {
        return max_val;
    }
    return value;
}

esp_err_t bno086_parse_rotation_vector(const uint8_t *payload, size_t length, bno086_rotation_vector_t *rotation)
{
    if (!payload || !rotation || length < 12)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t report_id = payload[0];
    if (report_id != 0x05 && report_id != 0x09) // rotation vector (0x05) or game rotation vector (0x09)
    {
        return ESP_ERR_INVALID_ARG;
    }
    rotation->status = payload[1];

    const float quat_scale = 1.0f / 16384.0f; // 2^-14
    int16_t qi = (int16_t)((payload[3] << 8) | payload[2]);
    int16_t qj = (int16_t)((payload[5] << 8) | payload[4]);
    int16_t qk = (int16_t)((payload[7] << 8) | payload[6]);
    int16_t qr = (int16_t)((payload[9] << 8) | payload[8]);

    rotation->x = (float)qi * quat_scale;
    rotation->y = (float)qj * quat_scale;
    rotation->z = (float)qk * quat_scale;
    rotation->w = (float)qr * quat_scale;

    int16_t acc_raw = (int16_t)((payload[11] << 8) | payload[10]);
    rotation->accuracy_radians = (float)acc_raw / 1024.0f; // 2^-10
    return ESP_OK;
}

void bno086_rotation_vector_to_euler(const bno086_rotation_vector_t *rotation, bno086_euler_t *euler)
{
    if (!rotation || !euler)
    {
        return;
    }
    float qw = rotation->w;
    float qx = rotation->x;
    float qy = rotation->y;
    float qz = rotation->z;

    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    euler->roll = atan2f(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (qw * qy - qz * qx);
    sinp = clampf(sinp, -1.0f, 1.0f);
    euler->pitch = asinf(sinp);

    float siny_cosp = 2.0f * (qw * qz + qx * qy);
    float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
    euler->yaw = atan2f(siny_cosp, cosy_cosp);
}

