#include "modbus_register_map.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

// Forward declarations for assembly buffers
extern uint8_t g_assembly_data064[32];  // Input Assembly 100
extern uint8_t g_assembly_data096[32];  // Output Assembly 150
extern uint8_t g_assembly_data097[10];  // Config Assembly 151

static const char *TAG = "modbus_regmap";
static SemaphoreHandle_t s_assembly_mutex = NULL;
static SemaphoreHandle_t s_mutex_init_mutex = NULL;

// Register address ranges
#define INPUT_REG_START        0
#define INPUT_REG_END          15
#define HOLDING_REG_OUTPUT_START 100
#define HOLDING_REG_OUTPUT_END   115
#define HOLDING_REG_CONFIG_START  150
#define HOLDING_REG_CONFIG_END    154

static uint16_t bytes_to_big_endian_uint16(const uint8_t *bytes)
{
    return (bytes[0] << 8) | bytes[1];
}

static void uint16_to_big_endian_bytes(uint16_t value, uint8_t *bytes)
{
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
}

static uint16_t little_endian_to_big_endian(uint16_t le_value)
{
    return ((le_value & 0xFF) << 8) | ((le_value >> 8) & 0xFF);
}

static uint16_t big_endian_to_little_endian(uint16_t be_value)
{
    return ((be_value & 0xFF) << 8) | ((be_value >> 8) & 0xFF);
}

static void ensure_assembly_mutex(void)
{
    if (s_assembly_mutex == NULL) {
        if (s_mutex_init_mutex == NULL) {
            s_mutex_init_mutex = xSemaphoreCreateMutex();
        }
        if (xSemaphoreTake(s_mutex_init_mutex, portMAX_DELAY) == pdTRUE) {
            if (s_assembly_mutex == NULL) {
                s_assembly_mutex = xSemaphoreCreateMutex();
            }
            xSemaphoreGive(s_mutex_init_mutex);
        }
    }
}

bool modbus_read_input_registers(uint16_t start_addr, uint16_t quantity, uint8_t *data)
{
    // Check bounds: start_addr must be within range and quantity must not overflow
    if (start_addr > INPUT_REG_END || quantity == 0 || start_addr + quantity > INPUT_REG_END + 1) {
        ESP_LOGW(TAG, "Invalid input register range: %d-%d", start_addr, start_addr + quantity - 1);
        return false;
    }
    
    ensure_assembly_mutex();
    if (s_assembly_mutex == NULL) {
        ESP_LOGE(TAG, "Assembly mutex not initialized");
        return false;
    }
    
    xSemaphoreTake(s_assembly_mutex, portMAX_DELAY);
    
    // Map Input Assembly 100 (32 bytes = 16 registers) to Modbus Input Registers 0-15
    // Assembly data is stored as little-endian bytes [low_byte, high_byte]
    // Modbus requires big-endian bytes [high_byte, low_byte]
    for (uint16_t i = 0; i < quantity; i++) {
        uint16_t reg_addr = start_addr + i;
        uint16_t byte_offset = reg_addr * 2;
        
        if (byte_offset + 1 < sizeof(g_assembly_data064)) {
            // Read little-endian from assembly: [low_byte, high_byte]
            uint8_t low_byte = g_assembly_data064[byte_offset];
            uint8_t high_byte = g_assembly_data064[byte_offset + 1];
            
            // Write as big-endian for Modbus: [high_byte, low_byte]
            data[i * 2] = high_byte;
            data[i * 2 + 1] = low_byte;
            
            ESP_LOGD(TAG, "Input reg %d: assembly[%d]=0x%02X, assembly[%d]=0x%02X -> Modbus[%d]=0x%02X, Modbus[%d]=0x%02X",
                     reg_addr, byte_offset, low_byte, byte_offset + 1, high_byte, i * 2, data[i * 2], i * 2 + 1, data[i * 2 + 1]);
        } else {
            // Out of range, return zero
            data[i * 2] = 0;
            data[i * 2 + 1] = 0;
        }
    }
    
    xSemaphoreGive(s_assembly_mutex);
    ESP_LOGD(TAG, "Read %d input registers starting at %d", quantity, start_addr);
    return true;
}

bool modbus_read_holding_registers(uint16_t start_addr, uint16_t quantity, uint8_t *data)
{
    // Check if address is in output assembly range (100-115)
    if (start_addr >= HOLDING_REG_OUTPUT_START && start_addr + quantity <= HOLDING_REG_OUTPUT_END + 1) {
        ensure_assembly_mutex();
        if (s_assembly_mutex == NULL) {
            return false;
        }
        
        xSemaphoreTake(s_assembly_mutex, portMAX_DELAY);
        
        // Map Output Assembly 150 (32 bytes = 16 registers) to Modbus Holding Registers 100-115
        uint16_t reg_offset = start_addr - HOLDING_REG_OUTPUT_START;
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t byte_offset = (reg_offset + i) * 2;
            
            if (byte_offset + 1 < sizeof(g_assembly_data096)) {
                uint16_t le_value = g_assembly_data096[byte_offset] | (g_assembly_data096[byte_offset + 1] << 8);
                uint16_t be_value = little_endian_to_big_endian(le_value);
                uint16_to_big_endian_bytes(be_value, &data[i * 2]);
            } else {
                uint16_to_big_endian_bytes(0, &data[i * 2]);
            }
        }
        
        xSemaphoreGive(s_assembly_mutex);
        return true;
    }
    
    // Check if address is in config assembly range (150-154)
    if (start_addr >= HOLDING_REG_CONFIG_START && start_addr + quantity <= HOLDING_REG_CONFIG_END + 1) {
        ensure_assembly_mutex();
        if (s_assembly_mutex == NULL) {
            return false;
        }
        
        xSemaphoreTake(s_assembly_mutex, portMAX_DELAY);
        
        // Map Config Assembly 151 (10 bytes = 5 registers) to Modbus Holding Registers 150-154
        uint16_t reg_offset = start_addr - HOLDING_REG_CONFIG_START;
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t byte_offset = (reg_offset + i) * 2;
            
            if (byte_offset + 1 < sizeof(g_assembly_data097)) {
                uint16_t le_value = g_assembly_data097[byte_offset] | (g_assembly_data097[byte_offset + 1] << 8);
                uint16_t be_value = little_endian_to_big_endian(le_value);
                uint16_to_big_endian_bytes(be_value, &data[i * 2]);
            } else {
                uint16_to_big_endian_bytes(0, &data[i * 2]);
            }
        }
        
        xSemaphoreGive(s_assembly_mutex);
        return true;
    }
    
    ESP_LOGW(TAG, "Invalid holding register range: %d-%d", start_addr, start_addr + quantity - 1);
    return false;
}

bool modbus_write_holding_register(uint16_t address, uint16_t value)
{
    uint8_t data[2];
    uint16_to_big_endian_bytes(value, data);
    return modbus_write_holding_registers(address, 1, data);
}

bool modbus_write_holding_registers(uint16_t start_addr, uint16_t quantity, const uint8_t *data)
{
    // Check if address is in output assembly range (100-115)
    if (start_addr >= HOLDING_REG_OUTPUT_START && start_addr + quantity <= HOLDING_REG_OUTPUT_END + 1) {
        ensure_assembly_mutex();
        if (s_assembly_mutex == NULL) {
            return false;
        }
        
        xSemaphoreTake(s_assembly_mutex, portMAX_DELAY);
        
        // Map Modbus Holding Registers 100-115 to Output Assembly 150 (32 bytes = 16 registers)
        uint16_t reg_offset = start_addr - HOLDING_REG_OUTPUT_START;
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t byte_offset = (reg_offset + i) * 2;
            
            if (byte_offset + 1 < sizeof(g_assembly_data096)) {
                // Convert big-endian from Modbus to little-endian for assembly
                uint16_t be_value = bytes_to_big_endian_uint16(&data[i * 2]);
                uint16_t le_value = big_endian_to_little_endian(be_value);
                g_assembly_data096[byte_offset] = le_value & 0xFF;
                g_assembly_data096[byte_offset + 1] = (le_value >> 8) & 0xFF;
            }
        }
        
        xSemaphoreGive(s_assembly_mutex);
        
        return true;
    }
    
    // Check if address is in config assembly range (150-154)
    if (start_addr >= HOLDING_REG_CONFIG_START && start_addr + quantity <= HOLDING_REG_CONFIG_END + 1) {
        ensure_assembly_mutex();
        if (s_assembly_mutex == NULL) {
            return false;
        }
        
        xSemaphoreTake(s_assembly_mutex, portMAX_DELAY);
        
        // Map Modbus Holding Registers 150-154 to Config Assembly 151 (10 bytes = 5 registers)
        uint16_t reg_offset = start_addr - HOLDING_REG_CONFIG_START;
        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t byte_offset = (reg_offset + i) * 2;
            
            if (byte_offset + 1 < sizeof(g_assembly_data097)) {
                uint16_t be_value = bytes_to_big_endian_uint16(&data[i * 2]);
                uint16_t le_value = big_endian_to_little_endian(be_value);
                g_assembly_data097[byte_offset] = le_value & 0xFF;
                g_assembly_data097[byte_offset + 1] = (le_value >> 8) & 0xFF;
            }
        }
        
        xSemaphoreGive(s_assembly_mutex);
        return true;
    }
    
    ESP_LOGW(TAG, "Invalid holding register range for write: %d-%d", start_addr, start_addr + quantity - 1);
    return false;
}

