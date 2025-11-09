# MCP4725 ESP-IDF Component

Utility functions for the MCP4725 12-bit I²C DAC. Supports writing to the DAC register and optionally storing the value in EEPROM.

## Directory Layout

```
components/mcp4725/
├── CMakeLists.txt
├── include/
│   └── mcp4725.h
└── mcp4725.c
```

## Public API

```c
bool mcp4725_init(mcp4725_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t mcp4725_write_dac(mcp4725_t *dev, uint16_t value);     // 12-bit (0-4095)
esp_err_t mcp4725_write_eeprom(mcp4725_t *dev, uint16_t value); // persists the value
esp_err_t mcp4725_write_dac_mode(mcp4725_t *dev, uint16_t value, mcp4725_power_mode_t mode);
esp_err_t mcp4725_set_power_mode(mcp4725_t *dev, mcp4725_power_mode_t mode);
esp_err_t mcp4725_read_status(mcp4725_t *dev, mcp4725_status_t *status);
```

## Example

```c
mcp4725_t dac;
mcp4725_init(&dac, dev_handle);

mcp4725_write_dac(&dac, 2048);        // mid-scale
mcp4725_write_eeprom(&dac, 2048);     // optional: store in EEPROM

mcp4725_set_power_mode(&dac, MCP4725_POWER_MODE_PD_1K); // power down with 1 kΩ to ground

mcp4725_status_t status;
mcp4725_read_status(&dac, &status);
```

The MCP4725 uses address `0x60` by default (A0 selects additional addresses). Remember that EEPROM writes take ~25ms; poll ACK or delay before issuing the next command.

