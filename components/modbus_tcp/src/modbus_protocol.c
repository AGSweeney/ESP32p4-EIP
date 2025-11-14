#include "modbus_protocol.h"
#include "modbus_register_map.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <string.h>
#include <errno.h>

static const char *TAG = "modbus_protocol";

// ModbusTCP ADU structure
typedef struct {
    uint16_t transaction_id;
    uint16_t protocol_id;      // Always 0 for Modbus
    uint16_t length;           // Length of unit identifier + PDU
    uint8_t unit_id;           // Slave address
    uint8_t function_code;
    uint8_t data[256];         // PDU data
} modbus_tcp_adu_t;

// Modbus function codes
#define MODBUS_FC_READ_HOLDING_REGISTERS   0x03
#define MODBUS_FC_READ_INPUT_REGISTERS     0x04
#define MODBUS_FC_WRITE_SINGLE_REGISTER    0x06
#define MODBUS_FC_WRITE_MULTIPLE_REGISTERS 0x10

// Exception codes
#define MODBUS_EX_ILLEGAL_FUNCTION          0x01
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS      0x02
#define MODBUS_EX_ILLEGAL_DATA_VALUE        0x03
#define MODBUS_EX_SLAVE_DEVICE_FAILURE      0x04


static void send_exception(int client_socket, uint16_t transaction_id, uint8_t unit_id, 
                          uint8_t function_code, uint8_t exception_code)
{
    uint8_t response[9];
    response[0] = (transaction_id >> 8) & 0xFF;
    response[1] = transaction_id & 0xFF;
    response[2] = 0x00; // Protocol ID
    response[3] = 0x00;
    response[4] = 0x00; // Length
    response[5] = 0x03;
    response[6] = unit_id;
    response[7] = function_code | 0x80; // Exception flag
    response[8] = exception_code;
    
    send(client_socket, response, sizeof(response), 0);
}

static bool handle_read_holding_registers(int client_socket, uint16_t transaction_id, 
                                         uint8_t unit_id, const uint8_t *pdu, int pdu_len)
{
    if (pdu_len < 5) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_READ_HOLDING_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_VALUE);
        return true;
    }
    
    uint16_t start_addr = (pdu[0] << 8) | pdu[1];
    uint16_t quantity = (pdu[2] << 8) | pdu[3];
    
    if (quantity == 0 || quantity > 125) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_READ_HOLDING_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_VALUE);
        return true;
    }
    
    uint8_t response[256];
    int response_len = 9 + quantity * 2;
    response[0] = (transaction_id >> 8) & 0xFF;
    response[1] = transaction_id & 0xFF;
    response[2] = 0x00; // Protocol ID
    response[3] = 0x00;
    response[4] = ((response_len - 6) >> 8) & 0xFF; // Length
    response[5] = (response_len - 6) & 0xFF;
    response[6] = unit_id;
    response[7] = MODBUS_FC_READ_HOLDING_REGISTERS;
    response[8] = quantity * 2; // Byte count
    
    // Read registers from map
    if (!modbus_read_holding_registers(start_addr, quantity, &response[9])) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_READ_HOLDING_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return true;
    }
    
    send(client_socket, response, response_len, 0);
    return true;
}

static bool handle_read_input_registers(int client_socket, uint16_t transaction_id, 
                                        uint8_t unit_id, const uint8_t *pdu, int pdu_len)
{
    if (pdu_len < 5) {
        ESP_LOGW(TAG, "Read input registers: PDU too short (%d bytes)", pdu_len);
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_READ_INPUT_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_VALUE);
        return true;
    }
    
    uint16_t start_addr = (pdu[0] << 8) | pdu[1];
    uint16_t quantity = (pdu[2] << 8) | pdu[3];
    
    ESP_LOGI(TAG, "Read input registers request: start_addr=%d, quantity=%d, unit_id=%d", 
             start_addr, quantity, unit_id);
    
    if (quantity == 0 || quantity > 125) {
        ESP_LOGW(TAG, "Invalid quantity: %d", quantity);
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_READ_INPUT_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_VALUE);
        return true;
    }
    
    uint8_t response[256];
    int response_len = 9 + quantity * 2;
    response[0] = (transaction_id >> 8) & 0xFF;
    response[1] = transaction_id & 0xFF;
    response[2] = 0x00; // Protocol ID
    response[3] = 0x00;
    response[4] = ((response_len - 6) >> 8) & 0xFF; // Length
    response[5] = (response_len - 6) & 0xFF;
    response[6] = unit_id;
    response[7] = MODBUS_FC_READ_INPUT_REGISTERS;
    response[8] = quantity * 2; // Byte count
    
    // Read registers from map
    if (!modbus_read_input_registers(start_addr, quantity, &response[9])) {
        ESP_LOGE(TAG, "Failed to read input registers: start_addr=%d, quantity=%d", 
                 start_addr, quantity);
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_READ_INPUT_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return true;
    }
    
    int sent = send(client_socket, response, response_len, 0);
    if (sent != response_len) {
        ESP_LOGW(TAG, "Failed to send full response: sent %d of %d bytes", sent, response_len);
    } else {
        ESP_LOGI(TAG, "Successfully sent input registers response: %d registers", quantity);
    }
    return true;
}

static bool handle_write_single_register(int client_socket, uint16_t transaction_id, 
                                         uint8_t unit_id, const uint8_t *pdu, int pdu_len)
{
    if (pdu_len < 5) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_WRITE_SINGLE_REGISTER, 
                      MODBUS_EX_ILLEGAL_DATA_VALUE);
        return true;
    }
    
    uint16_t address = (pdu[0] << 8) | pdu[1];
    uint16_t value = (pdu[2] << 8) | pdu[3];
    
    // Write register to map
    if (!modbus_write_holding_register(address, value)) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_WRITE_SINGLE_REGISTER, 
                      MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return true;
    }
    
    // Echo back the request
    uint8_t response[12];
    response[0] = (transaction_id >> 8) & 0xFF;
    response[1] = transaction_id & 0xFF;
    response[2] = 0x00; // Protocol ID
    response[3] = 0x00;
    response[4] = 0x00; // Length
    response[5] = 0x06;
    response[6] = unit_id;
    response[7] = MODBUS_FC_WRITE_SINGLE_REGISTER;
    response[8] = (address >> 8) & 0xFF;
    response[9] = address & 0xFF;
    response[10] = (value >> 8) & 0xFF;
    response[11] = value & 0xFF;
    
    send(client_socket, response, sizeof(response), 0);
    return true;
}

static bool handle_write_multiple_registers(int client_socket, uint16_t transaction_id, 
                                           uint8_t unit_id, const uint8_t *pdu, int pdu_len)
{
    if (pdu_len < 6) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_WRITE_MULTIPLE_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_VALUE);
        return true;
    }
    
    uint16_t start_addr = (pdu[0] << 8) | pdu[1];
    uint16_t quantity = (pdu[2] << 8) | pdu[3];
    uint8_t byte_count = pdu[4];
    
    if (quantity == 0 || quantity > 123 || byte_count != quantity * 2 || pdu_len < 6 + byte_count) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_WRITE_MULTIPLE_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_VALUE);
        return true;
    }
    
    // Write registers to map
    if (!modbus_write_holding_registers(start_addr, quantity, &pdu[5])) {
        send_exception(client_socket, transaction_id, unit_id, MODBUS_FC_WRITE_MULTIPLE_REGISTERS, 
                      MODBUS_EX_ILLEGAL_DATA_ADDRESS);
        return true;
    }
    
    // Response
    uint8_t response[12];
    response[0] = (transaction_id >> 8) & 0xFF;
    response[1] = transaction_id & 0xFF;
    response[2] = 0x00; // Protocol ID
    response[3] = 0x00;
    response[4] = 0x00; // Length
    response[5] = 0x06;
    response[6] = unit_id;
    response[7] = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
    response[8] = (start_addr >> 8) & 0xFF;
    response[9] = start_addr & 0xFF;
    response[10] = (quantity >> 8) & 0xFF;
    response[11] = quantity & 0xFF;
    
    send(client_socket, response, sizeof(response), 0);
    return true;
}

bool modbus_tcp_handle_request(int client_socket)
{
    uint8_t mbap_header[6];
    
    // Read MBAP header (6 bytes)
    int received = recv(client_socket, mbap_header, sizeof(mbap_header), 0);
    
    if (received <= 0) {
        if (received == 0 || errno == ECONNRESET) {
            ESP_LOGD(TAG, "Connection closed by client");
            return false; // Connection closed
        }
        ESP_LOGD(TAG, "Recv error: %s, retrying", strerror(errno));
        return true; // Try again
    }
    
    if (received < 6) {
        ESP_LOGD(TAG, "Partial MBAP header received: %d bytes", received);
        return true; // Wait for more data
    }
    
    uint16_t transaction_id = (mbap_header[0] << 8) | mbap_header[1];
    uint16_t protocol_id = (mbap_header[2] << 8) | mbap_header[3];
    uint16_t length = (mbap_header[4] << 8) | mbap_header[5];
    
    ESP_LOGD(TAG, "MBAP: transaction_id=%d, protocol_id=%d, length=%d", 
             transaction_id, protocol_id, length);
    
    if (protocol_id != 0) {
        ESP_LOGW(TAG, "Invalid protocol ID: %d (expected 0)", protocol_id);
        return false;
    }
    
    if (length < 2 || length > 253) {
        ESP_LOGW(TAG, "Invalid length: %d (must be 2-253)", length);
        return false;
    }
    
    // Read PDU (unit_id + function_code + data)
    uint8_t pdu[256];
    received = recv(client_socket, pdu, length, 0);
    if (received != length) {
        ESP_LOGW(TAG, "Failed to read full PDU: received %d of %d bytes", received, length);
        return false;
    }
    
    uint8_t unit_id = pdu[0];
    uint8_t function_code = pdu[1];
    const uint8_t *pdu_data = &pdu[2];
    int pdu_data_len = length - 2;
    
    ESP_LOGI(TAG, "Modbus request: unit_id=%d, function_code=0x%02X, pdu_len=%d", 
             unit_id, function_code, pdu_data_len);
    
    bool result = true;
    switch (function_code) {
        case MODBUS_FC_READ_HOLDING_REGISTERS:
            result = handle_read_holding_registers(client_socket, transaction_id, unit_id, pdu_data, pdu_data_len);
            break;
        case MODBUS_FC_READ_INPUT_REGISTERS:
            result = handle_read_input_registers(client_socket, transaction_id, unit_id, pdu_data, pdu_data_len);
            break;
        case MODBUS_FC_WRITE_SINGLE_REGISTER:
            result = handle_write_single_register(client_socket, transaction_id, unit_id, pdu_data, pdu_data_len);
            break;
        case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            result = handle_write_multiple_registers(client_socket, transaction_id, unit_id, pdu_data, pdu_data_len);
            break;
        default:
            ESP_LOGW(TAG, "Unsupported function code: 0x%02X", function_code);
            send_exception(client_socket, transaction_id, unit_id, function_code, MODBUS_EX_ILLEGAL_FUNCTION);
            result = true;
            break;
    }
    
    return result;
}

