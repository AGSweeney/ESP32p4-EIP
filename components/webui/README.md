# Web UI Component

A comprehensive web-based user interface for the ESP32-P4 OpENer EtherNet/IP adapter, providing real-time sensor monitoring, configuration management, and firmware updates.

## Overview

The Web UI component provides a modern, responsive web interface accessible via HTTP on port 80. It offers complete device configuration, real-time sensor data visualization, EtherNet/IP assembly monitoring, and over-the-air (OTA) firmware update capabilities.

## Features

- **Network Configuration**: Configure DHCP/Static IP, netmask, gateway, and DNS settings
- **VL53L1x Sensor Configuration**: Full sensor parameter configuration with calibration support
- **Real-time Sensor Monitoring**: Live distance readings with visual bar chart
- **EtherNet/IP Assembly Monitoring**: Bit-level visualization of Input and Output assemblies
- **Modbus TCP Configuration**: Enable/disable Modbus TCP server
- **OTA Firmware Updates**: Upload and install firmware updates via web interface
- **Responsive Design**: Works on desktop and mobile devices
- **No External Dependencies**: All CSS and JavaScript is self-contained (no CDN)

## Web Pages

### Configuration Page (`/`)
The main configuration page provides access to all device settings:

- **Network Configuration Card**
  - DHCP/Static IP mode selection
  - IP address, netmask, gateway configuration
  - DNS server configuration (hidden when using DHCP)
  - All settings stored in OpENer's NVS

- **Modbus TCP Card**
  - Enable/disable Modbus TCP server
  - Information about Modbus data mapping
  - Settings persist across reboots

- **VL53L1x Sensor Configuration Card**
  - Enable/disable sensor
  - Sensor data byte range selection (0-8, 9-17, or 18-26)
  - Distance mode (SHORT <1.3m, LONG <4m)
  - Timing budget configuration
  - Inter-measurement period
  - ROI (Region of Interest) settings
  - Offset and Crosstalk calibration
  - Signal/Sigma thresholds
  - Threshold window configuration
  - Interrupt polarity settings
  - I2C address configuration

### VL53L1x Status Page (`/vl53l1x`)
Real-time sensor monitoring dashboard:

- **Sensor Readings**
  - Distance (mm)
  - Status code with human-readable descriptions
  - Ambient light (kcps)
  - Signal per SPAD (kcps)
  - Number of SPADs

- **Visual Distance Chart**
  - Horizontal bar chart with gradient color (red to green)
  - Dynamic scaling based on distance mode
  - Updates every 250ms

- **Error Status**
  - Status code in parentheses
  - Human-readable error descriptions

### Input Assembly Page (`/inputassembly`)
Bit-level visualization of EtherNet/IP Input Assembly 100:

- 32 bytes displayed individually
- Each byte shows: `Byte X HEX (0xYY) | DEC ZZZ`
- Individual bit checkboxes (read-only)
- Blue checkboxes with white checkmarks when active
- Auto-refresh every 250ms

### Output Assembly Page (`/outputassembly`)
Bit-level visualization of EtherNet/IP Output Assembly 150:

- 32 bytes displayed individually
- Each byte shows: `Byte X HEX (0xYY) | DEC ZZZ`
- Individual bit checkboxes (read-only)
- Blue checkboxes with white checkmarks when active
- Auto-refresh every 250ms

### Firmware Update Page (`/ota`)
Over-the-air firmware update interface:

- File upload for firmware binary
- Progress indication
- Auto-redirect to home page after successful update
- Styled file input button matching application design

## REST API Endpoints

All API endpoints return JSON responses.

### Configuration Endpoints

#### `GET /api/config`
Get current VL53L1x sensor configuration.

**Response:**
```json
{
  "distance_mode": 2,
  "timing_budget_ms": 100,
  "inter_measurement_ms": 100,
  "roi_x_size": 16,
  "roi_y_size": 16,
  "roi_center_spad": 199,
  "offset_mm": 0,
  "xtalk_cps": 0,
  "signal_threshold_kcps": 1024,
  "sigma_threshold_mm": 15,
  "threshold_low_mm": 0,
  "threshold_high_mm": 0,
  "threshold_window": 0,
  "interrupt_polarity": 1,
  "i2c_address": 41
}
```

#### `POST /api/config`
Update VL53L1x sensor configuration.

**Request Body:**
```json
{
  "distance_mode": 2,
  "timing_budget_ms": 100,
  "inter_measurement_ms": 100,
  ...
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "Configuration saved successfully"
}
```

### Status Endpoints

#### `GET /api/status`
Get current sensor status and readings.

**Response:**
```json
{
  "distance_mm": 1234,
  "status": 0,
  "ambient_kcps": 5678,
  "sig_per_spad_kcps": 1234,
  "num_spads": 16,
  "distance_mode": 2,
  "input_assembly_100": {
    "raw_bytes": [0, 1, 2, ...],
    "distance_mm": 1234,
    "status": 0,
    ...
  },
  "output_assembly_150": {
    "raw_bytes": [0, 0, 0, ...]
  }
}
```

#### `GET /api/assemblies`
Get EtherNet/IP assembly data.

**Response:**
```json
{
  "input_assembly_100": {
    "distance_mm": 1234,
    "status": 0,
    "ambient_kcps": 5678,
    "sig_per_spad_kcps": 1234,
    "num_spads": 16
  }
}
```

### Calibration Endpoints

#### `POST /api/calibrate/offset`
Trigger offset calibration.

**Request Body:**
```json
{
  "target_distance_mm": 100
}
```

**Response:**
```json
{
  "status": "ok",
  "offset_mm": 5,
  "message": "Offset calibration completed"
}
```

#### `POST /api/calibrate/xtalk`
Trigger crosstalk calibration.

**Request Body:**
```json
{
  "target_distance_mm": 100
}
```

**Response:**
```json
{
  "status": "ok",
  "xtalk_cps": 1234,
  "message": "Crosstalk calibration completed"
}
```

### Network Configuration Endpoints

#### `GET /api/ipconfig`
Get current IP configuration.

**Response:**
```json
{
  "use_dhcp": true,
  "ip_address": "192.168.1.100",
  "netmask": "255.255.255.0",
  "gateway": "192.168.1.1",
  "dns1": "8.8.8.8",
  "dns2": "8.8.4.4"
}
```

#### `POST /api/ipconfig`
Update IP configuration.

**Request Body:**
```json
{
  "use_dhcp": false,
  "ip_address": "192.168.1.100",
  "netmask": "255.255.255.0",
  "gateway": "192.168.1.1",
  "dns1": "8.8.8.8",
  "dns2": "8.8.4.4"
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "IP configuration saved successfully. Reboot required to apply changes."
}
```

### Modbus Configuration Endpoints

#### `GET /api/modbus`
Get Modbus TCP enabled state.

**Response:**
```json
{
  "enabled": true
}
```

#### `POST /api/modbus`
Set Modbus TCP enabled state.

**Request Body:**
```json
{
  "enabled": true
}
```

**Response:**
```json
{
  "status": "ok",
  "enabled": true,
  "message": "Modbus configuration saved successfully"
}
```

### Sensor Control Endpoints

#### `GET /api/sensor/enabled`
Get sensor enabled state.

**Response:**
```json
{
  "enabled": true
}
```

#### `POST /api/sensor/enabled`
Enable or disable the VL53L1x sensor.

**Request Body:**
```json
{
  "enabled": true
}
```

**Response:**
```json
{
  "status": "ok",
  "enabled": true,
  "message": "Sensor state saved successfully"
}
```

#### `GET /api/sensor/byteoffset`
Get sensor data byte offset configuration.

**Response:**
```json
{
  "start_byte": 0,
  "end_byte": 8,
  "range": "0-8"
}
```

#### `POST /api/sensor/byteoffset`
Set sensor data byte offset (0, 9, or 18).

**Request Body:**
```json
{
  "start_byte": 9
}
```

**Response:**
```json
{
  "status": "ok",
  "start_byte": 9,
  "end_byte": 17,
  "range": "9-17",
  "message": "Sensor byte offset saved successfully"
}
```

### OTA Endpoints

#### `POST /api/ota/update`
Trigger OTA firmware update.

**Request:** Multipart form data with firmware file, or JSON with URL:
```json
{
  "url": "http://example.com/firmware.bin"
}
```

**Response:**
```json
{
  "status": "ok",
  "message": "OTA update started"
}
```

#### `GET /api/ota/status`
Get OTA update status.

**Response:**
```json
{
  "status": "idle",
  "progress": 0
}
```

### System Endpoints

#### `POST /api/reboot`
Reboot the device.

**Response:**
```json
{
  "status": "ok",
  "message": "Device will reboot in 2 seconds"
}
```

## Architecture

### Components

- **`webui.c`**: HTTP server initialization and page routing
- **`webui_html.c`**: HTML, CSS, and JavaScript for all web pages (embedded as C strings)
- **`webui_api.c`**: REST API endpoint handlers

### HTTP Server Configuration

- **Port**: 80
- **Max URI Handlers**: 25
- **Max Open Sockets**: 7
- **Stack Size**: 16KB
- **Task Priority**: 5
- **Core**: 1 (runs on same core as sensor task)
- **Max Request Header Length**: 1024 bytes

### Data Storage

- **Network Configuration**: Stored in OpENer's `g_tcpip` NVS namespace
- **Sensor Configuration**: Stored in `vl53l1x_config` NVS namespace
- **Modbus Configuration**: Stored in `system` NVS namespace
- **Sensor Enabled State**: Stored in `system` NVS namespace
- **Sensor Byte Offset**: Stored in `system` NVS namespace

### Sensor Data Mapping

The VL53L1x sensor data is written to Input Assembly 100 (`g_assembly_data064`) at a configurable byte offset:

- **Default**: Bytes 0-8
- **Alternative Options**: Bytes 9-17 or 18-26
- **Data Layout** (9 bytes):
  - Bytes 0-1: Distance (mm, little-endian)
  - Byte 2: Status
  - Bytes 3-4: Ambient (kcps, little-endian)
  - Bytes 5-6: Signal per SPAD (kcps, little-endian)
  - Bytes 7-8: Number of SPADs (little-endian)

When the sensor is disabled, the configured byte range is zeroed out.

## Usage

### Accessing the Web Interface

1. Ensure the ESP32-P4 device is powered on and connected to your network
2. Find the device's IP address (check serial monitor or DHCP server)
3. Open a web browser and navigate to `http://<device-ip>`
4. The Configuration page will load automatically

### Configuration Workflow

1. **Network Setup** (if needed):
   - Navigate to Configuration page
   - Configure IP settings in Network Configuration card
   - Click "Save Network Configuration"
   - Reboot device to apply changes

2. **Sensor Configuration**:
   - Navigate to Configuration page
   - Configure sensor parameters in VL53L1x Sensor Configuration card
   - Click "Save Sensor Configuration" or "Save Measurement Settings"
   - Changes take effect immediately

3. **Calibration**:
   - Place target at known distance
   - Enter target distance in Offset or Crosstalk field
   - Click "Calibrate" button
   - Sensor automatically restarts ranging after calibration

4. **Monitoring**:
   - Navigate to VL53L1x Status page for real-time readings
   - Navigate to Input/Output Assembly pages for bit-level data

### Firmware Update

1. Navigate to Firmware Update page (`/ota`)
2. Click "Choose File" and select firmware binary
3. Click "Start Update"
4. Wait for update to complete
5. Device will automatically reboot
6. Browser will redirect to home page

## Development

### Adding a New Page

1. Add HTML function in `webui_html.c`:
   ```c
   const char *webui_get_newpage_html(void)
   {
       return "<!DOCTYPE html>..."
   }
   ```

2. Register URI handler in `webui.c`:
   ```c
   httpd_uri_t newpage_uri = {
       .uri = "/newpage",
       .method = HTTP_GET,
       .handler = newpage_handler,
       .user_ctx = NULL
   };
   httpd_register_uri_handler(server_handle, &newpage_uri);
   ```

3. Implement handler function that calls the HTML getter

### Adding a New API Endpoint

1. Implement handler function in `webui_api.c`:
   ```c
   static esp_err_t api_get_newendpoint_handler(httpd_req_t *req)
   {
       // Implementation
   }
   ```

2. Register in `webui_register_api_handlers()`:
   ```c
   httpd_uri_t get_newendpoint_uri = {
       .uri = "/api/newendpoint",
       .method = HTTP_GET,
       .handler = api_get_newendpoint_handler,
       .user_ctx = NULL
   };
   httpd_register_uri_handler(server, &get_newendpoint_uri);
   ```

### Updating Preview Files

Preview HTML files in `webui_preview/` can be updated using the extraction script:

```bash
python update_webui_previews.py
```

This script extracts HTML from `webui_html.c` and updates all preview files.

## Notes

- All settings persist across reboots via NVS
- Network configuration changes require a reboot to take effect
- Sensor configuration changes take effect immediately
- The web UI has no external dependencies (no CDN, all assets embedded)
- The sensor navigation item is hidden when the sensor is disabled
- Sensor configuration fields are hidden when the sensor is disabled (info box remains visible)

## Footer

All pages display the footer:
```
OpENer Ethernet/IP for ESP32-P4 | Adam G Sweeney 11-15-2025
```

