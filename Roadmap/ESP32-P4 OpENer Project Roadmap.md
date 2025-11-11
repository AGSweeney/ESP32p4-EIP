# ESP32-P4 OpENer Project Roadmap

## Phase 1 – Core Stability

**Goal:** Deliver a reliable EtherNet/IP adapter implementation.

### Network and CIP

-  Complete and verify Address Conflict Detection (ACD).
-  Validate Identity, TCP/IP, and Ethernet Link objects with configurable Vendor ID, Device Type, Product Code, and Revision.
-  Provide default I/O assemblies:
  - Input Assembly 100 (0x64)
  - Output Assembly 150 (0x96)
  - Configuration Assembly 151 (0x97)
  - Fixed layout with defined bit and byte mapping.

### Configuration

-  Establish a single configuration source (Kconfig plus optional JSON/TOML on flash).
-  Configurable parameters: IP settings, identity fields, and I/O mapping.

### Error Handling and Logging

-  Standardize log tags (`EIP`, `IO`, `NET`, `DRV_xxx`).
-  Implement a central error/status structure exposed through a diagnostic CIP object or assembly.
-  Ensure clear fatal error messages for hardware or driver failures.

------

## Phase 2 – I/O Abstraction Layer

**Goal:** Provide a unified API between OpENer and physical I/O hardware.

### I/O Map

-  Create `io_map.h` defining total digital/analog I/O counts and driver ownership.
-  Align map layout with input/output assemblies.

### Driver Interface

Define a common driver structure:

```
typedef struct {
    const char *name;
    esp_err_t (*init)(void);
    esp_err_t (*read_inputs)(uint8_t *buf);
    esp_err_t (*write_outputs)(const uint8_t *buf);
} io_driver_t;
```

-  Register active drivers in a single table.

### Reference Implementation

-  Implement a reference hardware stack (e.g. MCP23S17 + ADS1115).
-  Fully test and document mapping between drivers and assemblies.

------

## Phase 3 – Peripheral Discovery and Health

**Goal:** Automatically detect, register, and monitor connected I/O modules.

### Bus Scanning

-  Scan I²C for known device addresses at startup.
-  Optionally probe SPI devices using ID registers.

### Device Registry

Maintain a list of detected devices:

```
typedef struct {
    const char *name;
    uint8_t bus_type;
    uint8_t addr;
    bool online;
} io_device_info_t;
```

-  Expose registry information through a custom CIP object or diagnostic assembly.

### Health Monitoring

-  Periodically verify device IDs and status.
-  Mark devices offline if communication fails and apply safe I/O values.
-  Publish device health bitmask through a diagnostic assembly.

------

## Phase 4 – Developer Experience

**Goal:** Simplify development, maintenance, and understanding of the system.

### Documentation

-  Add `docs/internal-roadmap.md`.
-  Add `docs/io-mapping.md` describing assembly layouts and bit allocations.

### Example Projects

-  `example_basic_adapter/`: minimal build using core I/O.
-  `example_sensors/`: includes sensors such as ToF, IMU, and load cell.

### Build Profiles

-  Provide `sdkconfig.defaults.basic` and `sdkconfig.defaults.full`.
-  Update README with build and flash instructions.

### Testing

-  Add simple Python host-side tests to verify Identity object, I/O connections, and data exchange.

------

## Phase 5 – Enhancements

**Goal:** Add extended functionality and usability features.

### Web Interface

-  Implement a local web dashboard displaying IP config, identity, connection status, and live I/O state.
-  Optional web-based output control for debugging.

### Time Synchronization

-  Include timestamps on sensor data.
-  Integrate SNTP or PTP for time alignment.

### Remote Configuration

-  Enable configuration through CIP Parameter objects or a REST endpoint.
-  Maintain backward-compatible local configuration files.

------

## OTA & WebUI Implementation Plan

### OTA Update Support

-  **ota-partitions:** Update `partitions.csv` (or equivalent) to add `ota_0`, `ota_1`, and `ota_data` entries sized for the current image (≥1 MB each).
-  **sdkconfig-ota:** Enable OTA-related Kconfig options (HTTPS client, OTA rollback handling, certificates) in `sdkconfig`.
-  **ota-module:** Create `components/ota_manager` with wrapper logic for fetching firmware via HTTP(S), reporting progress, validating, and triggering reboot.
-  **main-integration:** Hook OTA trigger into the main app (CLI command, timer, or diagnostic task) and add status logging.

### Web Dashboard

-  **http-server:** Enable ESP-IDF HTTP server component; add `components/webui` to serve REST endpoints and static assets.
-  **webui-pages:** Build single-page dashboard (HTML/JS/CSS) showing IP configuration, identity attributes, connection state, and live I/O data.
-  **webui-io-endpoints:** Expose JSON endpoints for current I/O assemblies; optionally provide POST endpoint to set outputs when a debug flag is enabled.
-  **auth-config:** Provide basic auth or token guard (configurable) for write endpoints to avoid accidental use.

### Documentation & Testing

-  **docs-update:** Extend `README.md` with OTA/WebUI usage instructions, API endpoints, and security considerations.
-  **test-plan:** Add manual/automated tests covering OTA download success/failure, rollback, and WebUI data refresh.

------

## Immediate Next Steps

1.  Finalize the I/O abstraction API (`io_init`, `io_read_inputs`, `io_write_outputs`) and link it to assemblies.
2.  Implement and validate one complete reference hardware path (MCP23S17 + ADC).
3.  Add I²C peripheral discovery and logging.
4.  Add `io-mapping.md` and roadmap documentation.
5.  Create a simple host-side CIP test script for automated validation.