# ESP32-P4 OpENer Project Roadmap

## Phase 1 – Core Stability ✅ **COMPLETED**

**Goal:** Deliver a reliable EtherNet/IP adapter implementation.

### Network and CIP ✅

-  ⏳ Complete and verify Address Conflict Detection (ACD) - RFC 5227 compliant implementation **IN PROGRESS**
  - ✅ RFC 5227 implementation completed in lwIP (`LWIP_ACD_RFC5227_COMPLIANT_STATIC`)
  - ✅ Static IP assignment deferred until ACD probe completes
  - ✅ ACD conflict detection and callback handling implemented
  - ⏳ Comprehensive testing and verification pending
  - ⏳ Edge case validation (conflict scenarios, network conditions) pending
-  ✅ Validate Identity, TCP/IP, and Ethernet Link objects with configurable Vendor ID, Device Type, Product Code, and Revision
-  ✅ Provide default I/O assemblies:
  - Input Assembly 100 (0x64) - 32 bytes, configurable sensor data byte range
  - Output Assembly 150 (0x96) - 32 bytes
  - Configuration Assembly 151 (0x97) - 10 bytes
  - Fixed layout with defined bit and byte mapping

### Configuration ✅

-  ✅ Establish NVS-based configuration storage (`system_config` component)
-  ✅ Configurable parameters: IP settings (DHCP/Static), identity fields, and I/O mapping
-  ✅ Web UI for runtime configuration (Network, Sensor, Modbus settings)
-  ✅ OpENer NVS integration for TCP/IP configuration persistence

### Error Handling and Logging ✅

-  ✅ Standardize log tags (`opener_main`, `webui_api`, `modbus_tcp`, `vl53l1x`, etc.)
-  ✅ Implement error/status reporting through Web UI API endpoints
-  ✅ Clear error messages for hardware or driver failures
-  ✅ Thread-safe error handling with mutex protection

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

## Phase 5 – Enhancements ✅ **MOSTLY COMPLETED**

**Goal:** Add extended functionality and usability features.

### Web Interface ✅

-  ✅ Implement a local web dashboard displaying IP config, identity, connection status, and live I/O state
-  ✅ Web-based sensor configuration (VL53L1x) with calibration support
-  ✅ Real-time sensor monitoring with visual bar chart
-  ✅ EtherNet/IP assembly monitoring (Input and Output assemblies with bit-level visualization)
-  ✅ Network configuration (DHCP/Static IP, netmask, gateway, DNS)
-  ✅ Modbus TCP enable/disable configuration
-  ✅ Sensor enable/disable and byte range configuration
-  ✅ Comprehensive documentation (`docs/WebUIReadme.md`)

### OTA Firmware Updates ✅

-  ✅ OTA partition support (`ota_0`, `ota_1`, `ota_data`)
-  ✅ Web-based firmware upload via multipart form data
-  ✅ Streaming OTA update support (no double buffering)
-  ✅ Progress tracking and status reporting
-  ✅ Automatic reboot after successful update

### Modbus TCP Integration ✅

-  ✅ Modbus TCP server implementation (port 502)
-  ✅ Mirrors EtherNet/IP assembly data structures
-  ✅ Input registers (0-15) map to Input Assembly 100
-  ✅ Holding registers (100-115) map to Output Assembly 150
-  ✅ Holding registers (150-154) map to Config Assembly 151
-  ✅ Web UI enable/disable control
-  ✅ Thread-safe assembly access with mutex protection

### Sensor Integration ✅

-  ✅ VL53L1x Time-of-Flight sensor driver integration
-  ✅ Sensor data written to configurable byte range in Input Assembly 100
-  ✅ Runtime sensor enable/disable
-  ✅ Offset and Crosstalk calibration via Web UI
-  ✅ Sensor configuration persistence in NVS
-  ✅ Configurable byte range (0-8, 9-17, or 18-26)

### Code Quality & Stability ✅

-  ✅ Memory leak review and fixes
-  ✅ Race condition fixes (assembly data access, sensor state variables)
-  ✅ Thread synchronization with mutexes
-  ✅ Comprehensive error handling

### Time Synchronization ⏳ **PENDING**

-  ⏳ Include timestamps on sensor data
-  ⏳ Integrate SNTP or PTP for time alignment

### Remote Configuration ✅

-  ✅ Enable configuration through REST API endpoints
-  ✅ Web UI for all configuration parameters
-  ✅ NVS-based persistent storage

------

## OTA & WebUI Implementation Plan ✅ **COMPLETED**

### OTA Update Support ✅

-  ✅ **ota-partitions:** OTA partitions configured in `partitions.csv` (`ota_0`, `ota_1`, `ota_data`)
-  ✅ **sdkconfig-ota:** OTA-related Kconfig options enabled
-  ✅ **ota-module:** `components/ota_manager` created with streaming update support, progress tracking, and reboot handling
-  ✅ **main-integration:** OTA integrated via Web UI API endpoints (`/api/ota/update`)
-  ✅ **web-integration:** Firmware upload page with file selection and progress display

### Web Dashboard ✅

-  ✅ **http-server:** ESP-IDF HTTP server component enabled; `components/webui` serves REST endpoints and static HTML/CSS/JS
-  ✅ **webui-pages:** Multi-page dashboard:
  - Configuration page (Network, Modbus, Sensor settings)
  - VL53L1x Status page (real-time sensor readings with bar chart)
  - Input Assembly page (T->O) with bit-level visualization
  - Output Assembly page (O->T) with bit-level visualization
  - Firmware Update page (OTA upload)
-  ✅ **webui-api-endpoints:** Comprehensive REST API:
  - `/api/status` - Sensor status and assembly data
  - `/api/config` - Sensor configuration (GET/POST)
  - `/api/ipconfig` - Network configuration (GET/POST)
  - `/api/modbus` - Modbus TCP enable/disable (GET/POST)
  - `/api/sensor/enabled` - Sensor enable/disable (GET/POST)
  - `/api/sensor/byteoffset` - Sensor data byte range (GET/POST)
  - `/api/calibrate/offset` - Offset calibration
  - `/api/calibrate/xtalk` - Crosstalk calibration
  - `/api/ota/update` - Firmware upload
  - `/api/ota/status` - OTA status
-  ✅ **no-auth:** Currently no authentication (suitable for local network use)

### Documentation & Testing ✅

-  ✅ **docs-update:** Comprehensive documentation:
  - `README.md` - Main project documentation with Web UI overview
  - `docs/WebUIReadme.md` - Complete Web UI user guide with screenshots
  - `components/webui/README.md` - Web UI component documentation
  - `docs/MEMORY_LEAKS_AND_RACE_CONDITIONS.md` - Code quality analysis
-  ✅ **component-organization:** Unused components moved to `components/unused/` with documentation
-  ⏳ **test-plan:** Manual testing completed; automated tests pending

------

## Immediate Next Steps

### High Priority

1.  ⏳ **ACD RFC 5227 Verification** - Complete comprehensive testing and validation of Address Conflict Detection implementation
   - Test conflict detection scenarios
   - Verify static IP assignment behavior
   - Validate edge cases and error handling
   - Document test results and compliance status
2.  ⏳ **I/O Abstraction Layer** - Create unified API between OpENer and physical I/O hardware (Phase 2)
3.  ⏳ **Peripheral Discovery** - Implement I²C/SPI device scanning and health monitoring (Phase 3)
4.  ⏳ **Time Synchronization** - Add SNTP/PTP support for sensor data timestamps
5.  ⏳ **Automated Testing** - Create host-side CIP test scripts for validation

### Medium Priority

6.  ⏳ **Example Projects** - Create `example_basic_adapter/` and `example_sensors/` projects
7.  ⏳ **Build Profiles** - Provide `sdkconfig.defaults.basic` and `sdkconfig.defaults.full`
8.  ⏳ **Documentation** - Add `docs/io-mapping.md` describing assembly layouts and bit allocations
9.  ⏳ **Web UI Enhancements** - Add authentication/authorization for write endpoints

### Low Priority

10. ⏳ **Additional Sensors** - Integrate other sensor types (IMU, load cell, etc.)
11. ⏳ **Advanced Features** - Implement CIP Parameter objects for remote configuration
12. ⏳ **Performance Optimization** - Profile and optimize critical paths

---

## Completed Work Summary

### ✅ Phase 1 - Core Stability (Mostly Complete)
- Network and CIP implementation with RFC 5227 ACD implementation (verification in progress)
- I/O assemblies (Input 100, Output 150, Config 151)
- NVS-based configuration system
- Standardized logging and error handling

### ✅ Phase 5 - Enhancements
- Comprehensive Web UI with 5 pages
- OTA firmware update support
- Modbus TCP server integration
- VL53L1x sensor integration
- Code quality improvements (memory leaks, race conditions)

### ✅ Documentation
- Main README with project overview
- Web UI user guide with screenshots
- Component documentation
- Code quality analysis report