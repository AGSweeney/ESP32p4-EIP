#include "vl53l1x.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "VL53L1X_api.h"
#include "VL53L1X_calibration.h"
#include "esp_log.h"

#include "i2c_device_handler.h"

static const char *TAG = "VL53L1X";

void wait_boot(uint16_t dev);
bool vl53l1x_init(vl53l1x_handle_t *vl53l1x_handle)
{
    g_vl53l1x_write_multi_ptr = i2c_write_multi;
    g_vl53l1x_read_multi_ptr = i2c_read_multi;

    g_vl53l1x_write_byte_ptr = i2c_write_byte;
    g_vl53l1x_write_word_ptr = i2c_write_word;
    g_vl53l1x_write_dword_ptr = i2c_write_dword;
    g_vl53l1x_read_byte_ptr = i2c_read_byte;
    g_vl53l1x_read_word_ptr = i2c_read_word;
    g_vl53l1x_read_dword_ptr = i2c_read_dword;

    // check if i2c is initialized
    if (!i2c_master_init(vl53l1x_handle->i2c_handle))
    {
        return false;
    }
    
    vl53l1x_handle->initialized = true;
    return true;
}

bool vl53l1x_add_device(vl53l1x_device_handle_t *device)
{
    if (!device->vl53l1x_handle->initialized)
    {
        ESP_LOGE(TAG, "attempted to add device while vl53l1x not initialized, call vl53l1x_init first");
        return false;
    }

    if (!i2c_add_device(device))
    {
        return false;
    }
    ESP_LOGI(TAG, "device 0x%04X created", device->dev);

    wait_boot(device->dev);
    ESP_LOGI(TAG, "device booted");

    VL53L1X_ERROR init_status = VL53L1X_SensorInit(device->dev);
    if (init_status != 0)
    {
        ESP_LOGE(TAG, "device failed initialization with error code: 0x%02X", init_status);
        return false;
    }
    ESP_LOGI(TAG, "device initialized successfully");

    // configuration
    VL53L1X_SetDistanceMode(device->dev, LONG);

    // calibraton
    VL53L1X_StartTemperatureUpdate(device->dev);
    // VL53L1X_CalibrateOffset(device->dev, 100, NULL);
    // VL53L1X_CalibrateXtalk(device->dev, 100, NULL);

    ESP_LOGI(TAG, "device ready");
    VL53L1X_StartRanging(device->dev);

    // log stuff after initialization success
    vl53l1x_log_sensor_id(device);
    vl53l1x_log_ambient_light(device);
    vl53l1x_log_signal_rate(device);
    return true;
}

uint16_t vl53l1x_get_mm(const vl53l1x_device_handle_t *device)
{
    uint16_t distance = 0;
    VL53L1X_GetDistance(device->dev, &distance);
    VL53L1X_ClearInterrupt(device->dev);
    return distance;
}

bool vl53l1x_update_device_address(vl53l1x_device_handle_t *device, uint8_t new_address)
{
    if (!i2c_update_address(device->dev, new_address))
    {
        return false;
    }

    ESP_LOGI(TAG, "device address updated: 0x%02X->0x%02X", device->i2c_address, new_address);
    device->dev = create_dev(get_port(device->dev), new_address);

    return true;
}

void vl53l1x_log_sensor_id(const vl53l1x_device_handle_t *device)
{
    uint16_t id = 0;
    VL53L1X_GetSensorId(device->dev, &id);

    ESP_LOGI(TAG, "Model ID: 0x%02X", id); // VL53L1X = 0xEEAC
}

void vl53l1x_log_ambient_light(const vl53l1x_device_handle_t *device)
{
    uint16_t ambRate;
    VL53L1X_GetAmbientRate(device->dev, &ambRate);

    ESP_LOGI(TAG, "Ambient rate: %dkcps", ambRate);
}

void vl53l1x_log_signal_rate(const vl53l1x_device_handle_t *device)
{
    uint16_t signal;
    VL53L1X_GetSignalRate(device->dev, &signal);
    ESP_LOGI(TAG, "Signal rate: %dkcps", signal);
}

bool vl53l1x_calibrate_offset(const vl53l1x_device_handle_t *device, uint16_t target_distance_mm, int16_t *offset)
{
    if (!device || !offset)
    {
        ESP_LOGE(TAG, "Invalid parameters for offset calibration");
        return false;
    }

    ESP_LOGI(TAG, "Starting offset calibration at %d mm...", target_distance_mm);
    int8_t status = VL53L1X_CalibrateOffset(device->dev, target_distance_mm, offset);
    
    if (status == 0)
    {
        ESP_LOGI(TAG, "Offset calibration successful: %d mm", *offset);
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "Offset calibration failed with error: %d", status);
        return false;
    }
}

bool vl53l1x_calibrate_xtalk(const vl53l1x_device_handle_t *device, uint16_t target_distance_mm, uint16_t *xtalk)
{
    if (!device || !xtalk)
    {
        ESP_LOGE(TAG, "Invalid parameters for xtalk calibration");
        return false;
    }

    ESP_LOGI(TAG, "Starting xtalk calibration at %d mm...", target_distance_mm);
    int8_t status = VL53L1X_CalibrateXtalk(device->dev, target_distance_mm, xtalk);
    
    if (status == 0)
    {
        ESP_LOGI(TAG, "Xtalk calibration successful: %d cps", *xtalk);
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "Xtalk calibration failed with error: %d", status);
        return false;
    }
}

bool vl53l1x_set_roi(const vl53l1x_device_handle_t *device, uint16_t x_size, uint16_t y_size)
{
    if (!device)
    {
        ESP_LOGE(TAG, "Invalid device handle for ROI setting");
        return false;
    }

    if (x_size > 16) x_size = 16;
    if (y_size > 16) y_size = 16;

    ESP_LOGI(TAG, "Setting ROI to %dx%d", x_size, y_size);
    VL53L1X_ERROR status = VL53L1X_SetROI(device->dev, x_size, y_size);
    
    if (status == 0)
    {
        ESP_LOGI(TAG, "ROI set successfully");
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "ROI setting failed with error: %d", status);
        return false;
    }
}

bool vl53l1x_set_roi_center(const vl53l1x_device_handle_t *device, uint8_t center_spad)
{
    if (!device)
    {
        ESP_LOGE(TAG, "Invalid device handle for ROI center setting");
        return false;
    }

    if (center_spad > 199)
    {
        ESP_LOGE(TAG, "Invalid ROI center SPAD: %d (max 199)", center_spad);
        return false;
    }

    ESP_LOGI(TAG, "Setting ROI center to SPAD %d", center_spad);
    VL53L1X_ERROR status = VL53L1X_SetROICenter(device->dev, center_spad);
    
    if (status == 0)
    {
        ESP_LOGI(TAG, "ROI center set successfully");
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "ROI center setting failed with error: %d", status);
        return false;
    }
}

bool vl53l1x_get_roi_center(const vl53l1x_device_handle_t *device, uint8_t *center_spad)
{
    if (!device || !center_spad)
    {
        ESP_LOGE(TAG, "Invalid parameters for ROI center reading");
        return false;
    }

    VL53L1X_ERROR status = VL53L1X_GetROICenter(device->dev, center_spad);
    
    if (status == 0)
    {
        ESP_LOGI(TAG, "ROI center: SPAD %d", *center_spad);
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "ROI center read failed with error: %d", status);
        return false;
    }
}

void wait_boot(uint16_t dev)
{
    uint8_t boot_state = 0xFF;
    VL53L1X_ERROR status = 0;
    int timeout_ms = 500;
    int elapsed_ms = 0;
    
    ESP_LOGI(TAG, "Waiting for sensor boot...");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while (elapsed_ms < timeout_ms)
    {
        status = VL53L1X_BootState(dev, &boot_state);
        if (status == 0 && boot_state == 0)
        {
            ESP_LOGI(TAG, "Sensor booted after %d ms (boot_state=%d)", elapsed_ms, boot_state);
            return;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
        elapsed_ms += 50;
    }
    
    ESP_LOGW(TAG, "Boot state check timeout (boot_state=%d, status=%d), proceeding anyway", boot_state, status);
}