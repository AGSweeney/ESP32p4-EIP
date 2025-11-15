# Unused Components

This directory contains components that are **not currently used or tested** in the ESP32-P4 OpENer EtherNet/IP adapter project.

## Components

The following components are included in this directory but are **not integrated** into the main application:

- **bmi270** - BMI270 6-axis IMU sensor driver
- **bno086** - BNO086 9-axis IMU sensor driver  
- **hc165** - 74HC165 8-bit parallel-in serial-out shift register driver
- **hc595** - 74HC595 8-bit serial-in parallel-out shift register driver
- **hx711** - HX711 load cell amplifier driver
- **lsm6dsv16x** - LSM6DSV16X 6-axis IMU sensor driver
- **mcp23008** - MCP23008 8-bit I/O expander driver
- **mcp23017** - MCP23017 16-bit I/O expander driver
- **mcp3008** - MCP3008 10-bit ADC driver
- **mcp3208** - MCP3208 12-bit ADC driver
- **mcp4725** - MCP4725 12-bit DAC driver
- **nau7802** - NAU7802 24-bit ADC driver
- **pcf8574** - PCF8574 8-bit I/O expander driver
- **pcf8575** - PCF8575 16-bit I/O expander driver
- **tca9534** - TCA9534 8-bit I/O expander driver
- **tca9555** - TCA9555 16-bit I/O expander driver
- **vl53l0x** - VL53L0X time-of-flight sensor driver (replaced by VL53L1X)

## Status

These components are provided for **reference purposes only**. They have been:
- ❌ **Not tested** with the current ESP32-P4 OpENer implementation
- ❌ **Not integrated** into the main application code
- ❌ **Not verified** for compatibility with the current hardware configuration

## Usage

If you wish to use any of these components:

1. Move the component directory back to `components/`
2. Add it to `main/CMakeLists.txt` in the `REQUIRES` or `PRIV_REQUIRES` section
3. Include the component's header files in your code
4. Test thoroughly before using in production

## Currently Used Components

The following components **are** actively used in the project:

- **opener** - OpENer EtherNet/IP stack
- **webui** - Web user interface
- **modbus_tcp** - Modbus TCP server
- **system_config** - System configuration (NVS)
- **ota_manager** - Over-the-air firmware updates
- **vl53l1x_uld** - VL53L1X sensor driver (Ultra-Lite Driver)
- **vl53l1x_config** - VL53L1X configuration management

