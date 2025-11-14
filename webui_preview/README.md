# Web UI Preview

This directory contains standalone HTML files that preview what the web interface will look like when running on the ESP32-P4 device.

## Files

- **index.html** - Main configuration page for VL53L1x sensor settings
- **status.html** - Real-time sensor status dashboard
- **ota.html** - OTA firmware update page

## Usage

Simply open `index.html` in your web browser to preview the interface. The pages use mock data and simulated API responses, so you can see the layout and functionality without needing the actual device.

## Features Preview

### Configuration Page (index.html)
- All VL53L1x sensor configuration options
- Form validation
- Save/Load/Reset functionality (simulated)
- Calibration buttons (simulated)
- Navigation to Status and OTA pages

### Status Page (status.html)
- Real-time sensor readings (simulated with random updates)
- Current configuration display
- Auto-refresh every second
- Navigation links

### OTA Page (ota.html)
- File upload interface
- Progress bar simulation
- Status messages
- Navigation links

## Note

This is a **preview only**. The actual API calls are simulated with mock data. When running on the device, these pages will connect to the real REST API endpoints.

