#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "lwip/ip4_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IP configuration structure
 */
typedef struct {
    bool use_dhcp;              // true for DHCP, false for static
    uint32_t ip_address;        // IP address in network byte order
    uint32_t netmask;           // Network mask in network byte order
    uint32_t gateway;           // Gateway in network byte order
    uint32_t dns1;              // Primary DNS in network byte order
    uint32_t dns2;              // Secondary DNS in network byte order
} system_ip_config_t;

/**
 * @brief Get default IP configuration (DHCP enabled)
 */
void system_ip_config_get_defaults(system_ip_config_t *config);

/**
 * @brief Load IP configuration from NVS
 * @param config Pointer to config structure to fill
 * @return true if loaded successfully, false if using defaults
 */
bool system_ip_config_load(system_ip_config_t *config);

/**
 * @brief Save IP configuration to NVS
 * @param config Pointer to config structure to save
 * @return true on success, false on error
 */
bool system_ip_config_save(const system_ip_config_t *config);

/**
 * @brief Load Modbus enabled state from NVS
 * @return true if Modbus is enabled, false if disabled or not set
 */
bool system_modbus_enabled_load(void);

/**
 * @brief Save Modbus enabled state to NVS
 * @param enabled true to enable Modbus, false to disable
 * @return true on success, false on error
 */
bool system_modbus_enabled_save(bool enabled);

/**
 * @brief Load VL53L1x sensor enabled state from NVS
 * @return true if sensor is enabled, false if disabled or not set
 */
bool system_sensor_enabled_load(void);

/**
 * @brief Save VL53L1x sensor enabled state to NVS
 * @param enabled true to enable sensor, false to disable
 * @return true on success, false on error
 */
bool system_sensor_enabled_save(bool enabled);

/**
 * @brief Load VL53L1x sensor data start byte offset from NVS
 * @return Start byte offset (0, 9, or 18). Defaults to 0 if not set or invalid.
 */
uint8_t system_sensor_byte_offset_load(void);

/**
 * @brief Save VL53L1x sensor data start byte offset to NVS
 * @param start_byte Start byte offset (must be 0, 9, or 18)
 * @return true on success, false on error or invalid value
 */
bool system_sensor_byte_offset_save(uint8_t start_byte);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_CONFIG_H

