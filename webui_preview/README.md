# Web UI Preview

This directory contains standalone HTML files that preview what the web interface will look like when running on the ESP32-P4 device.

## Files

- **index.html** - Configuration page with Network, Sensor, and Modbus settings
- **status.html** (VL53L1x) - Real-time sensor status dashboard with distance bar chart
- **inputassembly.html** (T->O) - Input Assembly 100 bit-level display
- **outputassembly.html** (O->T) - Output Assembly 150 bit-level display
- **ota.html** - OTA firmware update page

## Usage

Simply open any HTML file in your web browser to preview the interface. These are exact copies of the HTML served by the device, so you can see the layout and styling without needing the actual device.

**Note:** These files contain the actual HTML/CSS/JavaScript but API calls will fail when opened locally. To see full functionality, you need to access the pages through the running ESP32-P4 device.

## Features Preview

### Configuration Page (index.html)
- **Network Configuration Card**: DHCP/Static IP settings, DNS configuration
- **VL53L1x Sensor Configuration Card**: All sensor settings, calibration options
- **Modbus TCP Card**: Enable/disable Modbus server
- Navigation to all pages

### Status Page (status.html /vl53l1x)
- Real-time sensor readings (distance, status, ambient, signal per SPAD, number of SPADs)
- Distance range bar chart with gradient color (red to green)
- Dynamic scaling based on distance mode (1300mm for short, 4000mm for long)
- Error status descriptions with human-readable messages
- Auto-refresh every 250ms

### Input Assembly Page (inputassembly.html /inputassembly)
- Bit-level display of Input Assembly 100 (32 bytes)
- Individual checkboxes for each bit (read-only)
- Blue checkboxes with white checkmarks when active
- Byte display format: "Byte X HEX (0xYY) | DEC ZZZ"

### Output Assembly Page (outputassembly.html /outputassembly)
- Bit-level display of Output Assembly 150 (32 bytes)
- Individual checkboxes for each bit (read-only)
- Blue checkboxes with white checkmarks when active
- Byte display format: "Byte X HEX (0xYY) | DEC ZZZ"

### OTA Page (ota.html)
- File upload interface for firmware updates
- Styled file input button
- Status messages
- Auto-redirect to home page after successful update

## Updating Preview Files

To update these preview files after making changes to the web UI, run:
```bash
python update_webui_previews.py
```

This script extracts the HTML from `components/webui/src/webui_html.c` and updates all preview files.

