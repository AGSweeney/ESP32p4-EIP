# ESP32-P4 OpENer Project Roadmap

## Phase 1 – Core Stability

**Goal:** Deliver a reliable EtherNet/IP adapter implementation.

### Network and CIP

-  Complete and verify Address Conflict Detection (ACD).
-  Validate Identity, TCP/IP, and Ethernet Link objects with configurable Vendor ID, Device Type, Product Code, and Revision.
-  Provide default I/O assemblies:
  - Input Assembly 0x64
  - Output Assembly 0x65
  - Configuration Assembly 0x66
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

## Immediate Next Steps

1.  Finalize the I/O abstraction API (`io_init`, `io_read_inputs`, `io_write_outputs`) and link it to assemblies.
2.  Implement and validate one complete reference hardware path (MCP23S17 + ADC).
3.  Add I²C peripheral discovery and logging.
4.  Add `io-mapping.md` and roadmap documentation.
5.  Create a simple host-side CIP test script for automated validation.