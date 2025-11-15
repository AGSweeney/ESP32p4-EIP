#include "webui_api.h"
#include "vl53l1x_config.h"
#include "ota_manager.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for assembly access
extern uint8_t g_assembly_data064[32];
extern uint8_t g_assembly_data096[32];
extern uint8_t g_assembly_data097[10];

// Forward declaration for device handle (will be set by sampleapplication)
extern void *g_vl53l1x_device_handle;

static const char *TAG = "webui_api";

// Helper function to send JSON response
static esp_err_t send_json_response(httpd_req_t *req, cJSON *json, esp_err_t status_code)
{
    char *json_str = cJSON_Print(json);
    if (json_str == NULL) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, status_code == ESP_OK ? "200 OK" : "400 Bad Request");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(json);
    return ESP_OK;
}

// Helper function to send JSON error response
static esp_err_t send_json_error(httpd_req_t *req, const char *message, int http_status)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "status", "error");
    cJSON_AddStringToObject(json, "message", message);
    
    char *json_str = cJSON_Print(json);
    if (json_str == NULL) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    if (http_status == 400) {
        httpd_resp_set_status(req, "400 Bad Request");
    } else if (http_status == 500) {
        httpd_resp_set_status(req, "500 Internal Server Error");
    } else {
        httpd_resp_set_status(req, "400 Bad Request");
    }
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    cJSON_Delete(json);
    return ESP_OK;
}

// GET /api/config - Get current VL53L1x configuration
static esp_err_t api_get_config_handler(httpd_req_t *req)
{
    vl53l1x_config_t config;
    vl53l1x_config_get_defaults(&config);
    vl53l1x_config_load(&config); // Try to load, falls back to defaults if not found
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "distance_mode", config.distance_mode);
    cJSON_AddNumberToObject(json, "timing_budget_ms", config.timing_budget_ms);
    cJSON_AddNumberToObject(json, "inter_measurement_ms", config.inter_measurement_ms);
    cJSON_AddNumberToObject(json, "roi_x_size", config.roi_x_size);
    cJSON_AddNumberToObject(json, "roi_y_size", config.roi_y_size);
    cJSON_AddNumberToObject(json, "roi_center_spad", config.roi_center_spad);
    cJSON_AddNumberToObject(json, "offset_mm", config.offset_mm);
    cJSON_AddNumberToObject(json, "xtalk_cps", config.xtalk_cps);
    cJSON_AddNumberToObject(json, "signal_threshold_kcps", config.signal_threshold_kcps);
    cJSON_AddNumberToObject(json, "sigma_threshold_mm", config.sigma_threshold_mm);
    cJSON_AddNumberToObject(json, "threshold_low_mm", config.threshold_low_mm);
    cJSON_AddNumberToObject(json, "threshold_high_mm", config.threshold_high_mm);
    cJSON_AddNumberToObject(json, "threshold_window", config.threshold_window);
    cJSON_AddNumberToObject(json, "interrupt_polarity", config.interrupt_polarity);
    cJSON_AddNumberToObject(json, "i2c_address", config.i2c_address);
    
    return send_json_response(req, json, ESP_OK);
}

// POST /api/config - Update VL53L1x configuration
static esp_err_t api_post_config_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    vl53l1x_config_t config;
    vl53l1x_config_get_defaults(&config);
    
    // Parse JSON and update config
    cJSON *item;
    if ((item = cJSON_GetObjectItem(json, "distance_mode")) != NULL) {
        config.distance_mode = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "timing_budget_ms")) != NULL) {
        config.timing_budget_ms = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "inter_measurement_ms")) != NULL) {
        config.inter_measurement_ms = (uint32_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "roi_x_size")) != NULL) {
        config.roi_x_size = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "roi_y_size")) != NULL) {
        config.roi_y_size = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "roi_center_spad")) != NULL) {
        config.roi_center_spad = (uint8_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "offset_mm")) != NULL) {
        config.offset_mm = (int16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "xtalk_cps")) != NULL) {
        config.xtalk_cps = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "signal_threshold_kcps")) != NULL) {
        config.signal_threshold_kcps = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "sigma_threshold_mm")) != NULL) {
        config.sigma_threshold_mm = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "threshold_low_mm")) != NULL) {
        config.threshold_low_mm = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "threshold_high_mm")) != NULL) {
        config.threshold_high_mm = (uint16_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "threshold_window")) != NULL) {
        config.threshold_window = (uint8_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "interrupt_polarity")) != NULL) {
        config.interrupt_polarity = (uint8_t)cJSON_GetNumberValue(item);
    }
    if ((item = cJSON_GetObjectItem(json, "i2c_address")) != NULL) {
        config.i2c_address = (uint8_t)cJSON_GetNumberValue(item);
    }
    
    cJSON_Delete(json);
    
    // Validate configuration
    if (!vl53l1x_config_validate(&config)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid configuration values");
        return ESP_FAIL;
    }
    
    // Save configuration
    if (!vl53l1x_config_save(&config)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // Apply configuration to sensor if device handle is available
    if (g_vl53l1x_device_handle != NULL) {
        extern bool vl53l1x_apply_config(void *device, const void *config);
        if (!vl53l1x_apply_config(g_vl53l1x_device_handle, &config)) {
            ESP_LOGW(TAG, "Failed to apply configuration to sensor");
        }
    }
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "ok");
    cJSON_AddStringToObject(response, "message", "Configuration saved successfully");
    
    return send_json_response(req, response, ESP_OK);
}

// POST /api/reboot - Reboot the device
static esp_err_t api_reboot_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Reboot requested via web UI");
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "ok");
    cJSON_AddStringToObject(response, "message", "Device rebooting...");
    
    esp_err_t ret = send_json_response(req, response, ESP_OK);
    
    // Give a small delay to ensure response is sent
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Reboot the device
    esp_restart();
    
    return ret; // This will never be reached
}

// GET /api/status - Get sensor status and current readings
static esp_err_t api_get_status_handler(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateObject();
    
    // Read sensor data from assembly buffer
    uint16_t distance = g_assembly_data064[0] | (g_assembly_data064[1] << 8);
    uint8_t status = g_assembly_data064[2];
    uint16_t ambient = g_assembly_data064[3] | (g_assembly_data064[4] << 8);
    uint16_t sig_per_spad = g_assembly_data064[5] | (g_assembly_data064[6] << 8);
    uint16_t num_spads = g_assembly_data064[7] | (g_assembly_data064[8] << 8);
    
    cJSON_AddNumberToObject(json, "distance_mm", distance);
    cJSON_AddNumberToObject(json, "status", status);
    cJSON_AddNumberToObject(json, "ambient_kcps", ambient);
    cJSON_AddNumberToObject(json, "sig_per_spad_kcps", sig_per_spad);
    cJSON_AddNumberToObject(json, "num_spads", num_spads);
    
    return send_json_response(req, json, ESP_OK);
}

// GET /api/assemblies - Get EtherNet/IP assembly data
static esp_err_t api_get_assemblies_handler(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateObject();
    
    // Input Assembly 100
    cJSON *input_assembly = cJSON_CreateObject();
    uint16_t distance = g_assembly_data064[0] | (g_assembly_data064[1] << 8);
    uint8_t status = g_assembly_data064[2];
    uint16_t ambient = g_assembly_data064[3] | (g_assembly_data064[4] << 8);
    uint16_t sig_per_spad = g_assembly_data064[5] | (g_assembly_data064[6] << 8);
    uint16_t num_spads = g_assembly_data064[7] | (g_assembly_data064[8] << 8);
    
    cJSON_AddNumberToObject(input_assembly, "distance_mm", distance);
    cJSON_AddNumberToObject(input_assembly, "status", status);
    cJSON_AddNumberToObject(input_assembly, "ambient_kcps", ambient);
    cJSON_AddNumberToObject(input_assembly, "sig_per_spad_kcps", sig_per_spad);
    cJSON_AddNumberToObject(input_assembly, "num_spads", num_spads);
    cJSON_AddItemToObject(json, "input_assembly_100", input_assembly);
    
    // Output Assembly 150
    cJSON *output_assembly = cJSON_CreateObject();
    uint8_t led_control = g_assembly_data096[0] & 0x01;
    cJSON_AddBoolToObject(output_assembly, "led", led_control != 0);
    cJSON_AddItemToObject(json, "output_assembly_150", output_assembly);
    
    // Config Assembly 151
    cJSON *config_assembly = cJSON_CreateObject();
    cJSON_AddNumberToObject(config_assembly, "size", sizeof(g_assembly_data097));
    cJSON_AddItemToObject(json, "config_assembly_151", config_assembly);
    
    return send_json_response(req, json, ESP_OK);
}

// POST /api/calibrate/offset - Trigger offset calibration
static esp_err_t api_calibrate_offset_handler(httpd_req_t *req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *item = cJSON_GetObjectItem(json, "target_distance_mm");
    if (item == NULL) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing target_distance_mm");
        return ESP_FAIL;
    }
    
    uint16_t target_distance = (uint16_t)cJSON_GetNumberValue(item);
    cJSON_Delete(json);
    
    if (g_vl53l1x_device_handle == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Sensor not initialized");
        return ESP_FAIL;
    }
    
    extern bool vl53l1x_calibrate_offset(void *device, uint16_t target_distance_mm, int16_t *offset);
    int16_t offset = 0;
    bool success = vl53l1x_calibrate_offset(g_vl53l1x_device_handle, target_distance, &offset);
    
    cJSON *response = cJSON_CreateObject();
    if (success) {
        cJSON_AddStringToObject(response, "status", "ok");
        cJSON_AddNumberToObject(response, "offset_mm", offset);
        cJSON_AddStringToObject(response, "message", "Offset calibration successful");
        
        // Save updated offset to config
        vl53l1x_config_t config;
        vl53l1x_config_get_defaults(&config);
        vl53l1x_config_load(&config);
        config.offset_mm = offset;
        vl53l1x_config_save(&config);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Offset calibration failed");
    }
    
    return send_json_response(req, response, success ? ESP_OK : ESP_FAIL);
}

// POST /api/calibrate/xtalk - Trigger xtalk calibration
static esp_err_t api_calibrate_xtalk_handler(httpd_req_t *req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *item = cJSON_GetObjectItem(json, "target_distance_mm");
    if (item == NULL) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing target_distance_mm");
        return ESP_FAIL;
    }
    
    uint16_t target_distance = (uint16_t)cJSON_GetNumberValue(item);
    cJSON_Delete(json);
    
    if (g_vl53l1x_device_handle == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Sensor not initialized");
        return ESP_FAIL;
    }
    
    extern bool vl53l1x_calibrate_xtalk(void *device, uint16_t target_distance_mm, uint16_t *xtalk);
    uint16_t xtalk = 0;
    bool success = vl53l1x_calibrate_xtalk(g_vl53l1x_device_handle, target_distance, &xtalk);
    
    cJSON *response = cJSON_CreateObject();
    if (success) {
        cJSON_AddStringToObject(response, "status", "ok");
        cJSON_AddNumberToObject(response, "xtalk_cps", xtalk);
        cJSON_AddStringToObject(response, "message", "Xtalk calibration successful");
        
        // Save updated xtalk to config
        vl53l1x_config_t config;
        vl53l1x_config_get_defaults(&config);
        vl53l1x_config_load(&config);
        config.xtalk_cps = xtalk;
        vl53l1x_config_save(&config);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Xtalk calibration failed");
    }
    
    return send_json_response(req, response, success ? ESP_OK : ESP_FAIL);
}

// POST /api/ota/update - Trigger OTA update (supports both URL and file upload)
static esp_err_t api_ota_update_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "OTA update request received");
    
    // Check content type
    char content_type[256];
    esp_err_t ret = httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Missing Content-Type header (ret=%d)", ret);
        return send_json_error(req, "Missing Content-Type", 400);
    }
    
    ESP_LOGI(TAG, "OTA update request, Content-Type: %s", content_type);
    
    // Handle file upload (multipart/form-data) - Use streaming to avoid memory issues
    if (strstr(content_type, "multipart/form-data") != NULL) {
        // Get content length - may be 0 if chunked transfer
        size_t content_len = req->content_len;
        ESP_LOGI(TAG, "Content-Length: %d", content_len);
        
        // Validate size
        if (content_len > 2 * 1024 * 1024) { // Max 2MB for safety
            ESP_LOGW(TAG, "Content length too large: %d", content_len);
            return send_json_error(req, "File too large (max 2MB)", 400);
        }
        
        // Parse multipart boundary first
        const char *boundary_str = strstr(content_type, "boundary=");
        if (boundary_str == NULL) {
            ESP_LOGW(TAG, "No boundary found in Content-Type");
            return send_json_error(req, "Invalid multipart data: no boundary", 400);
        }
        boundary_str += 9; // Skip "boundary="
        
        // Extract boundary value
        char boundary[128];
        int boundary_len = 0;
        while (*boundary_str && *boundary_str != ';' && *boundary_str != ' ' && *boundary_str != '\r' && *boundary_str != '\n' && boundary_len < 127) {
            boundary[boundary_len++] = *boundary_str++;
        }
        boundary[boundary_len] = '\0';
        ESP_LOGI(TAG, "Multipart boundary: %s", boundary);
        
        // Use a small buffer to read multipart headers (64KB should be enough)
        const size_t header_buffer_size = 64 * 1024;
        char *header_buffer = malloc(header_buffer_size);
        if (header_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for header buffer");
            return send_json_error(req, "Failed to allocate memory", 500);
        }
        
        // Read enough to find the data separator (\r\n\r\n)
        size_t header_read = 0;
        bool found_separator = false;
        while (header_read < header_buffer_size - 1) {
            int ret = httpd_req_recv(req, header_buffer + header_read, header_buffer_size - header_read - 1);
            if (ret <= 0) {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                    continue;
                }
                ESP_LOGE(TAG, "Error reading headers: %d", ret);
                free(header_buffer);
                return send_json_error(req, "Failed to read request headers", 500);
            }
            header_read += ret;
            header_buffer[header_read] = '\0'; // Null terminate for string search
            
            // Look for data separator
            if (strstr(header_buffer, "\r\n\r\n") != NULL || strstr(header_buffer, "\n\n") != NULL) {
                found_separator = true;
                break;
            }
        }
        
        if (!found_separator) {
            ESP_LOGW(TAG, "Could not find data separator in multipart headers");
            free(header_buffer);
            return send_json_error(req, "Invalid multipart format: no data separator", 400);
        }
        
        // Find where data starts
        char *data_start = strstr(header_buffer, "\r\n\r\n");
        size_t header_len = 0;
        if (data_start != NULL) {
            header_len = (data_start - header_buffer) + 4;
        } else {
            data_start = strstr(header_buffer, "\n\n");
            if (data_start != NULL) {
                header_len = (data_start - header_buffer) + 2;
            } else {
                free(header_buffer);
                return send_json_error(req, "Invalid multipart format", 400);
            }
        }
        
        // Calculate how much data we already have in the buffer
        size_t data_in_buffer = header_read - header_len;
        
        // Start streaming OTA update
        // Estimate firmware size: Content-Length minus multipart headers (typically ~1KB)
        // This gives us a reasonable estimate for progress tracking
        size_t estimated_firmware_size = 0;
        if (content_len > 0) {
            // Subtract estimated multipart header overhead (boundary + headers ~1KB)
            estimated_firmware_size = (content_len > 1024) ? (content_len - 1024) : content_len;
        }
        esp_ota_handle_t ota_handle = ota_manager_start_streaming_update(estimated_firmware_size);
        if (ota_handle == 0) {
            ESP_LOGE(TAG, "Failed to start streaming OTA update - check serial logs for details");
            free(header_buffer);
            return send_json_error(req, "Failed to start OTA update. Check device logs for details.", 500);
        }
        
        // Write data we already have in buffer
        if (data_in_buffer > 0) {
            if (!ota_manager_write_streaming_chunk(ota_handle, (const uint8_t *)(header_buffer + header_len), data_in_buffer)) {
                ESP_LOGE(TAG, "Failed to write initial chunk");
                free(header_buffer);
                return send_json_error(req, "Failed to write firmware data", 500);
            }
        }
        
        free(header_buffer); // Free header buffer, we'll use a smaller chunk buffer now
        
        // Stream remaining data in chunks (64KB chunks)
        const size_t chunk_size = 64 * 1024;
        char *chunk_buffer = malloc(chunk_size);
        if (chunk_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate chunk buffer");
            return send_json_error(req, "Failed to allocate memory", 500);
        }
        
        size_t total_written = data_in_buffer;
        bool done = false;
        
        while (!done) {
            int ret = httpd_req_recv(req, chunk_buffer, chunk_size);
            if (ret <= 0) {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                    continue;
                }
                // Connection closed or error - check if we hit boundary
                done = true;
                break;
            }
            
            // Check for end boundary in this chunk
            char end_boundary[256];
            snprintf(end_boundary, sizeof(end_boundary), "\r\n--%s--", boundary);
            char *boundary_pos = strstr(chunk_buffer, end_boundary);
            if (boundary_pos == NULL) {
                snprintf(end_boundary, sizeof(end_boundary), "\r\n--%s", boundary);
                boundary_pos = strstr(chunk_buffer, end_boundary);
            }
            
            size_t to_write = ret;
            if (boundary_pos != NULL) {
                // Found boundary - only write up to it
                to_write = boundary_pos - chunk_buffer;
                // Remove trailing \r\n if present
                while (to_write > 0 && (chunk_buffer[to_write - 1] == '\r' || chunk_buffer[to_write - 1] == '\n')) {
                    to_write--;
                }
                done = true;
            }
            
            if (to_write > 0) {
                if (!ota_manager_write_streaming_chunk(ota_handle, (const uint8_t *)chunk_buffer, to_write)) {
                    ESP_LOGE(TAG, "Failed to write chunk at offset %d", total_written);
                    free(chunk_buffer);
                    return send_json_error(req, "Failed to write firmware data", 500);
                }
                total_written += to_write;
            }
        }
        
        free(chunk_buffer);
        
        ESP_LOGI(TAG, "Streamed %d bytes to OTA partition", total_written);
        
        // Finish OTA update (this will set boot partition and reboot)
        // Send HTTP response BEFORE finishing, as the device will reboot
        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "status", "ok");
        cJSON_AddStringToObject(response, "message", "Firmware uploaded successfully. Finishing update and rebooting...");
        
        // Send response first
        esp_err_t resp_err = send_json_response(req, response, ESP_OK);
        
        // Small delay to ensure response is sent
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Now finish the update (this will reboot)
        if (!ota_manager_finish_streaming_update(ota_handle)) {
            ESP_LOGE(TAG, "Failed to finish streaming OTA update");
            // Response already sent, but update failed - device will not reboot
            return ESP_FAIL;
        }
        
        // This should never be reached as ota_manager_finish_streaming_update() reboots
        return resp_err;
    }
    
    // Handle URL-based update (existing JSON method)
    if (strstr(content_type, "application/json") == NULL) {
        ESP_LOGW(TAG, "Unsupported Content-Type for OTA update: %s", content_type);
        return send_json_error(req, "Unsupported Content-Type. Use multipart/form-data for file upload or application/json for URL", 400);
    }
    
    char content[256];
    int bytes_received = httpd_req_recv(req, content, sizeof(content) - 1);
    if (bytes_received <= 0) {
        ESP_LOGE(TAG, "Failed to read request body");
        return send_json_error(req, "Failed to read request body", 500);
    }
    content[bytes_received] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGW(TAG, "Invalid JSON in request");
        return send_json_error(req, "Invalid JSON", 400);
    }
    
    cJSON *item = cJSON_GetObjectItem(json, "url");
    if (item == NULL || !cJSON_IsString(item)) {
        cJSON_Delete(json);
        return send_json_error(req, "Missing or invalid URL", 400);
    }
    
    const char *url = cJSON_GetStringValue(item);
    if (url == NULL) {
        cJSON_Delete(json);
        return send_json_error(req, "Invalid URL", 400);
    }
    cJSON_Delete(json);
    
    ESP_LOGI(TAG, "Starting OTA update from URL: %s", url);
    bool success = ota_manager_start_update(url);
    
    cJSON *response = cJSON_CreateObject();
    if (success) {
        cJSON_AddStringToObject(response, "status", "ok");
        cJSON_AddStringToObject(response, "message", "OTA update started");
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to start OTA update");
    }
    
    return send_json_response(req, response, success ? ESP_OK : ESP_FAIL);
}

// GET /api/ota/status - Get OTA status
static esp_err_t api_ota_status_handler(httpd_req_t *req)
{
    ota_status_info_t status_info;
    if (!ota_manager_get_status(&status_info)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON *json = cJSON_CreateObject();
    const char *status_str;
    switch (status_info.status) {
        case OTA_STATUS_IDLE:
            status_str = "idle";
            break;
        case OTA_STATUS_IN_PROGRESS:
            status_str = "in_progress";
            break;
        case OTA_STATUS_COMPLETE:
            status_str = "complete";
            break;
        case OTA_STATUS_ERROR:
            status_str = "error";
            break;
        default:
            status_str = "unknown";
            break;
    }
    
    cJSON_AddStringToObject(json, "status", status_str);
    cJSON_AddNumberToObject(json, "progress", status_info.progress);
    cJSON_AddStringToObject(json, "message", status_info.message);
    
    return send_json_response(req, json, ESP_OK);
}

void webui_register_api_handlers(httpd_handle_t server)
{
    // GET /api/config
    httpd_uri_t get_config_uri = {
        .uri       = "/api/config",
        .method    = HTTP_GET,
        .handler   = api_get_config_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &get_config_uri);
    
    // POST /api/config
    httpd_uri_t post_config_uri = {
        .uri       = "/api/config",
        .method    = HTTP_POST,
        .handler   = api_post_config_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &post_config_uri);
    
    // GET /api/status
    httpd_uri_t get_status_uri = {
        .uri       = "/api/status",
        .method    = HTTP_GET,
        .handler   = api_get_status_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &get_status_uri);
    
    // GET /api/assemblies
    httpd_uri_t get_assemblies_uri = {
        .uri       = "/api/assemblies",
        .method    = HTTP_GET,
        .handler   = api_get_assemblies_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &get_assemblies_uri);
    
    // POST /api/calibrate/offset
    httpd_uri_t calibrate_offset_uri = {
        .uri       = "/api/calibrate/offset",
        .method    = HTTP_POST,
        .handler   = api_calibrate_offset_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &calibrate_offset_uri);
    
    // POST /api/calibrate/xtalk
    httpd_uri_t calibrate_xtalk_uri = {
        .uri       = "/api/calibrate/xtalk",
        .method    = HTTP_POST,
        .handler   = api_calibrate_xtalk_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &calibrate_xtalk_uri);
    
    // POST /api/ota/update
    httpd_uri_t ota_update_uri = {
        .uri       = "/api/ota/update",
        .method    = HTTP_POST,
        .handler   = api_ota_update_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &ota_update_uri);
    
    // GET /api/ota/status
    httpd_uri_t ota_status_uri = {
        .uri       = "/api/ota/status",
        .method    = HTTP_GET,
        .handler   = api_ota_status_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &ota_status_uri);
    
    // POST /api/reboot
    httpd_uri_t reboot_uri = {
        .uri       = "/api/reboot",
        .method    = HTTP_POST,
        .handler   = api_reboot_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &reboot_uri);
}

