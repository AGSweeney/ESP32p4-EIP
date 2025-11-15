#include "system_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "system_config";
static const char *NVS_NAMESPACE = "system";
static const char *NVS_KEY_IPCONFIG = "ipconfig";
static const char *NVS_KEY_MODBUS_ENABLED = "modbus_enabled";
static const char *NVS_KEY_SENSOR_ENABLED = "sensor_enabled";
static const char *NVS_KEY_SENSOR_BYTE_OFFSET = "sens_byte_off";

void system_ip_config_get_defaults(system_ip_config_t *config)
{
    if (config == NULL) {
        return;
    }
    
    memset(config, 0, sizeof(system_ip_config_t));
    config->use_dhcp = true;  // Default to DHCP
    // All other fields are 0 (DHCP will assign)
}

bool system_ip_config_load(system_ip_config_t *config)
{
    if (config == NULL) {
        return false;
    }
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved IP configuration found, using defaults");
            system_ip_config_get_defaults(config);
            return false;
        }
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }
    
    size_t required_size = sizeof(system_ip_config_t);
    err = nvs_get_blob(handle, NVS_KEY_IPCONFIG, config, &required_size);
    nvs_close(handle);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved IP configuration found, using defaults");
        system_ip_config_get_defaults(config);
        return false;
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load IP configuration: %s", esp_err_to_name(err));
        return false;
    }
    
    if (required_size != sizeof(system_ip_config_t)) {
        ESP_LOGW(TAG, "IP configuration size mismatch (expected %zu, got %zu), using defaults",
                 sizeof(system_ip_config_t), required_size);
        system_ip_config_get_defaults(config);
        return false;
    }
    
    ESP_LOGI(TAG, "IP configuration loaded successfully from NVS (DHCP=%s)", 
             config->use_dhcp ? "enabled" : "disabled");
    return true;
}

bool system_ip_config_save(const system_ip_config_t *config)
{
    if (config == NULL) {
        return false;
    }
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }
    
    err = nvs_set_blob(handle, NVS_KEY_IPCONFIG, config, sizeof(system_ip_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save IP configuration: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit IP configuration: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "IP configuration saved successfully to NVS");
    return true;
}

bool system_modbus_enabled_load(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved Modbus enabled state found, defaulting to enabled");
            return true;  // Default to enabled
        }
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return true;  // Default to enabled on error
    }
    
    uint8_t enabled = 1;  // Default to enabled
    size_t required_size = sizeof(uint8_t);
    err = nvs_get_blob(handle, NVS_KEY_MODBUS_ENABLED, &enabled, &required_size);
    nvs_close(handle);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved Modbus enabled state found, defaulting to enabled");
        return true;  // Default to enabled
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load Modbus enabled state: %s", esp_err_to_name(err));
        return true;  // Default to enabled on error
    }
    
    ESP_LOGI(TAG, "Modbus enabled state loaded from NVS: %s", enabled ? "enabled" : "disabled");
    return enabled != 0;
}

bool system_modbus_enabled_save(bool enabled)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }
    
    uint8_t enabled_val = enabled ? 1 : 0;
    err = nvs_set_blob(handle, NVS_KEY_MODBUS_ENABLED, &enabled_val, sizeof(uint8_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save Modbus enabled state: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit Modbus enabled state: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Modbus enabled state saved successfully to NVS: %s", enabled ? "enabled" : "disabled");
    return true;
}

bool system_sensor_enabled_load(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved sensor enabled state found, defaulting to enabled");
            return true;  // Default to enabled
        }
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return true;  // Default to enabled on error
    }
    
    uint8_t enabled = 1;  // Default to enabled
    size_t required_size = sizeof(uint8_t);
    err = nvs_get_blob(handle, NVS_KEY_SENSOR_ENABLED, &enabled, &required_size);
    nvs_close(handle);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved sensor enabled state found, defaulting to enabled");
        return true;  // Default to enabled
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load sensor enabled state: %s", esp_err_to_name(err));
        return true;  // Default to enabled on error
    }
    
    ESP_LOGI(TAG, "Sensor enabled state loaded from NVS: %s", enabled ? "enabled" : "disabled");
    return enabled != 0;
}

bool system_sensor_enabled_save(bool enabled)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }
    
    uint8_t enabled_val = enabled ? 1 : 0;
    err = nvs_set_blob(handle, NVS_KEY_SENSOR_ENABLED, &enabled_val, sizeof(uint8_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save sensor enabled state: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit sensor enabled state: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Sensor enabled state saved successfully to NVS: %s", enabled ? "enabled" : "disabled");
    return true;
}

uint8_t system_sensor_byte_offset_load(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved sensor byte offset found, defaulting to 0");
            return 0;  // Default to 0
        }
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return 0;  // Default to 0 on error
    }
    
    uint8_t start_byte = 0;  // Default to 0
    size_t required_size = sizeof(uint8_t);
    err = nvs_get_blob(handle, NVS_KEY_SENSOR_BYTE_OFFSET, &start_byte, &required_size);
    nvs_close(handle);
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved sensor byte offset found, defaulting to 0");
        return 0;  // Default to 0
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load sensor byte offset: %s", esp_err_to_name(err));
        return 0;  // Default to 0 on error
    }
    
    // Validate: must be 0, 9, or 18
    if (start_byte != 0 && start_byte != 9 && start_byte != 18) {
        ESP_LOGW(TAG, "Invalid sensor byte offset %d found in NVS, defaulting to 0", start_byte);
        return 0;
    }
    
    ESP_LOGI(TAG, "Sensor byte offset loaded from NVS: %d (bytes %d-%d)", start_byte, start_byte, start_byte + 8);
    return start_byte;
}

bool system_sensor_byte_offset_save(uint8_t start_byte)
{
    // Validate: must be 0, 9, or 18
    if (start_byte != 0 && start_byte != 9 && start_byte != 18) {
        ESP_LOGE(TAG, "Invalid sensor byte offset: %d (must be 0, 9, or 18)", start_byte);
        return false;
    }
    
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }
    
    err = nvs_set_blob(handle, NVS_KEY_SENSOR_BYTE_OFFSET, &start_byte, sizeof(uint8_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save sensor byte offset: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit sensor byte offset: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Sensor byte offset saved successfully to NVS: %d (bytes %d-%d)", start_byte, start_byte, start_byte + 8);
    return true;
}

