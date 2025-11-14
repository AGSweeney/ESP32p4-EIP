#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_ota_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OTA_STATUS_IDLE,
    OTA_STATUS_IN_PROGRESS,
    OTA_STATUS_COMPLETE,
    OTA_STATUS_ERROR
} ota_status_t;

typedef struct {
    ota_status_t status;
    uint8_t progress;  // 0-100
    char message[128];
} ota_status_info_t;

/**
 * @brief Initialize OTA manager
 * 
 * @return true on success, false on error
 */
bool ota_manager_init(void);

/**
 * @brief Start OTA update from URL
 * 
 * @param url Firmware URL (HTTP or HTTPS)
 * @return true if update started successfully, false on error
 */
bool ota_manager_start_update(const char *url);

/**
 * @brief Start OTA update from uploaded binary data
 * 
 * @param data Pointer to binary firmware data
 * @param data_len Length of firmware data in bytes
 * @return true if update started successfully, false on error
 */
bool ota_manager_start_update_from_data(const uint8_t *data, size_t data_len);

/**
 * @brief Start streaming OTA update (writes directly to partition, no double buffering)
 * 
 * @param expected_size Expected firmware size in bytes (for validation)
 * @return esp_ota_handle_t OTA handle on success, 0 on error
 */
esp_ota_handle_t ota_manager_start_streaming_update(size_t expected_size);

/**
 * @brief Write chunk of firmware data to streaming OTA update
 * 
 * @param ota_handle OTA handle from ota_manager_start_streaming_update
 * @param data Pointer to data chunk
 * @param len Length of data chunk
 * @return true on success, false on error
 */
bool ota_manager_write_streaming_chunk(esp_ota_handle_t ota_handle, const uint8_t *data, size_t len);

/**
 * @brief Finish streaming OTA update
 * 
 * @param ota_handle OTA handle from ota_manager_start_streaming_update
 * @return true on success, false on error
 */
bool ota_manager_finish_streaming_update(esp_ota_handle_t ota_handle);

/**
 * @brief Get current OTA status
 * 
 * @param status_info Pointer to status structure to populate
 * @return true on success, false on error
 */
bool ota_manager_get_status(ota_status_info_t *status_info);

#ifdef __cplusplus
}
#endif

#endif // OTA_MANAGER_H

