# Address Conflict Detection (ACD) Testing Guide

## Overview

This document provides detailed instructions for testing the Address Conflict Detection (ACD) functionality in the ESP32-P4 OpENer project. ACD implements RFC 5227 compliant static IP address conflict detection, ensuring that static IP addresses are not assigned until the network confirms they are safe to use.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Understanding ACD in This Project](#understanding-acd-in-this-project)
3. [Test Setup](#test-setup)
4. [Test Scenarios](#test-scenarios)
5. [Verification Methods](#verification-methods)
6. [Expected Log Messages](#expected-log-messages)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Hardware Requirements
- ESP32-P4 development board with Ethernet connection
- Ethernet cable connected to a network switch/hub
- A second device (PC, another ESP32, or network device) capable of using the test IP address
- Serial console access (USB-UART) for monitoring logs

### Software Requirements
- ESP-IDF v5.5.1 or compatible
- EtherNet/IP client tool (e.g., RSLinx, Wireshark, or custom CIP client)
- Serial terminal program (e.g., PuTTY, minicom, or ESP-IDF monitor)

### Build Configuration
Ensure the following are enabled in `sdkconfig`:
- `CONFIG_LWIP_AUTOIP=y` (enables ACD support)
- `CONFIG_LWIP_DHCP_DOES_ACD_CHECK=y` (for DHCP ACD checking)
- `LWIP_ACD_RFC5227_COMPLIANT_STATIC=1` (default, enables RFC 5227 compliance)
- `CONFIG_OPENER_ACD_CUSTOM_TIMING=y` (enables ODVA-optimized ACD timings)

---

## Understanding ACD in This Project

### How ACD Works

1. **RFC 5227 Compliant Mode** (default):
   - IP address is **NOT assigned** until ACD confirms it's safe
   - ACD probe sequence runs before IP assignment
   - If conflict detected: IP is never assigned
   - If no conflict: IP is assigned after `ACD_IP_OK` callback

2. **ACD Configuration**:
   - Controlled via CIP TCP/IP Interface Object, Attribute #10 (`select_acd`)
   - Value `0` = ACD disabled (immediate IP assignment)
   - Value `1` = ACD enabled (RFC 5227 compliant deferred assignment)
   - Configuration persists to NVS and survives reboots

3. **Static IP from NVS**:
   - Static IP configuration is stored in NVS namespace `"opener"`, key `"tcpip_cfg"`
   - Loaded at boot via `NvTcpipLoad()`
   - Includes: IP address, netmask, gateway, DNS servers, and `select_acd` flag

### ACD Timing Configuration

This project implements **adjustable ACD timings** to comply with ODVA (Open DeviceNet Vendors Association) recommendations for EtherNet/IP devices. The timings are optimized for real-time industrial communication requirements.

#### ODVA Reference

**ODVA Publication 28 - Recommended IP Addressing Methods for EtherNet/IP™ Devices**
- **Purpose**: Best practices for IP address configuration in EtherNet/IP devices
- **Content**: Recommends Address Conflict Detection (ACD) for static IP assignment per RFC 5227
- **Direct Link**: [ODVA Document Library](https://www.odva.org/technology-standards/document-library/) (Search for "Pub 28" or "IP Addressing Methods")
- **Relevance**: ODVA recommends ACD for static IP assignment but emphasizes faster timing for real-time industrial applications

#### Current Configuration (Testing)

The project uses **custom ACD timings optimized/Testing for EtherNet/IP** via Kconfig options (`OPENER_ACD_CUSTOM_TIMING`). These values are significantly faster than RFC 5227 defaults to meet EtherNet/IP's real-time requirements:

| Parameter | RFC 5227 Default | **Testing (Current)** | Description |
|-----------|------------------|------------------------------|-------------|
| **PROBE_WAIT** | 1000 ms | **100 ms** | Initial random delay before first probe |
| **PROBE_MIN** | 1000 ms | **180 ms** | Minimum delay between probe packets |
| **PROBE_MAX** | 2000 ms | **200 ms** | Maximum delay between probe packets |
| **PROBE_NUM** | 3 packets | **4 packet** | Number of probe packets to send |
| **ANNOUNCE_WAIT** | 2000 ms | **200 ms** | Delay before first announcement |
| **ANNOUNCE_NUM** | 2 packets | **1 packet** | Number of announcement packets |
| **ANNOUNCE_INTERVAL** | 2000 ms | **200 ms** | Time between announcement packets |



#### Configuration

ACD timings are configurable via ESP-IDF menuconfig:
- Navigate to: `Component config > OpenER ACD Timing`
- Enable `Override default RFC5227 timings` to use ODVA-optimized values
- Adjust individual timing parameters as needed for your network environment

**Note**: The optimized timings balance conflict detection reliability with EtherNet/IP's real-time communication requirements. For networks with high latency or packet loss, you may need to increase these values.

---

## Test Setup

### Step 1: Configure Static IP in NVS

#### Option A: Using CIP SetAttributeSingle Service

1. Connect EtherNet/IP client to the device
2. Set TCP/IP Interface Object (Class 0xF5, Instance 1) attributes:
   - **Attribute #3** (`config_control`): Set bit 0 = 1 (Static IP mode)
   - **Attribute #5** (`interface_configuration`): Set IP address, netmask, gateway
   - **Attribute #10** (`select_acd`): Set to `1` (enable ACD)

#### Option B: Using ESP-IDF NVS API (Development)

Create a test program or use `idf.py monitor` with custom commands to write NVS:

```c
// Example: Write static IP configuration to NVS
nvs_handle_t handle;
nvs_open("opener", NVS_READWRITE, &handle);

TcpipNvBlob blob = {
    .version = 2,
    .config_control = 0x01,  // Static IP mode
    .ip_address = 0xC0A80164,  // 192.168.1.100
    .network_mask = 0xFFFFFF00, // 255.255.255.0
    .gateway = 0xC0A80101,     // 192.168.1.1
    .name_server = 0,
    .name_server2 = 0,
    .select_acd = 1  // Enable ACD
};

nvs_set_blob(handle, "tcpip_cfg", &blob, sizeof(blob));
nvs_commit(handle);
nvs_close(handle);
```

### Step 2: Prepare Test Network

1. **Choose a test IP address** (e.g., `192.168.1.100`)
2. **Ensure the network segment is isolated** or use a dedicated test network
3. **Prepare a conflict device**:
   - Configure another device with the same IP address
   - Or use ARP spoofing tools to simulate conflicts

### Step 3: Monitor Serial Output

Start ESP-IDF monitor to view logs:

```bash
idf.py monitor
```

Or use a serial terminal at 115200 baud.

---

## Test Scenarios

### Test Scenario 1: ACD Enabled - No Conflict (Success Case)

**Objective**: Verify that ACD successfully assigns IP when no conflict exists.

**Steps**:
1. Ensure no device on the network uses IP `192.168.1.100`
2. Configure ESP32 with static IP `192.168.1.100` and ACD enabled (`select_acd=1`)
3. Power cycle or reset the ESP32
4. Monitor serial output for ACD progress

**Expected Behavior**:
- ACD probe sequence runs (3 probe packets)
- No conflicts detected
- IP address is assigned after `ACD_IP_OK` callback
- Device becomes operational with assigned IP

**Expected Log Sequence**:
```
I (xxxx) opener_main: After NV load select_acd=1
I (xxxx) opener_main: ACD path: select_acd=1, RFC5227=1, lwip_netif=0x...
I (xxxx) opener_main: Using RFC 5227 compliant ACD for static IP
I (xxxx) opener_main: RFC 5227: ACD started, IP assignment deferred
I (xxxx) opener_main: ACD client registered
I (xxxx) opener_main: Starting ACD probe via acd_start()
I (xxxx) opener_main: ACD callback state=0
I (xxxx) opener_main: RFC 5227: IP assigned after ACD confirmation
```

**Verification**:
- Check TCP/IP Interface Object Attribute #1 (`status`): Should show IP configured
- Ping the device: `ping 192.168.1.100` should succeed
- Check Attribute #11 (`last_acd_activity`): Should be `0` (no conflict)

---

### Test Scenario 2: ACD Enabled - Conflict Detected

**Objective**: Verify that ACD detects conflicts and prevents IP assignment.

**Steps**:
1. Configure another device (PC or second ESP32) with IP `192.168.1.100`
2. Ensure the conflicting device is active and responds to ARP
3. Configure ESP32 with static IP `192.168.1.100` and ACD enabled
4. Power cycle or reset the ESP32
5. Monitor serial output

**Expected Behavior**:
- ACD probe sequence runs
- Conflict detected during probe phase
- IP address is **NOT assigned** (RFC 5227 compliant)
- Device reports ACD fault status

**Expected Log Sequence**:
```
I (xxxx) opener_main: After NV load select_acd=1
I (xxxx) opener_main: ACD path: select_acd=1, RFC5227=1, lwip_netif=0x...
I (xxxx) opener_main: Using RFC 5227 compliant ACD for static IP
I (xxxx) opener_main: RFC 5227: ACD started, IP assignment deferred
I (xxxx) opener_main: ACD client registered
I (xxxx) opener_main: Starting ACD probe via acd_start()
I (xxxx) opener_main: ACD callback state=1  (or state=2 for DECLINE)
W (xxxx) opener_main: RFC 5227: IP not assigned due to conflict
```

**Verification**:
- Check TCP/IP Interface Object Attribute #1 (`status`):
  - Bit `kTcpipStatusAcdStatus` should be set
  - Bit `kTcpipStatusAcdFault` should be set
- Check Attribute #11 (`last_acd_activity`): Should be `3` (conflict detected)
- Ping the device: Should fail (IP not assigned)
- Device should not respond to EtherNet/IP connections

---

### Test Scenario 3: ACD Disabled - Immediate Assignment

**Objective**: Verify that disabling ACD allows immediate IP assignment.

**Steps**:
1. Configure ESP32 with static IP `192.168.1.100` and ACD disabled (`select_acd=0`)
2. Power cycle or reset the ESP32
3. Monitor serial output

**Expected Behavior**:
- No ACD probe sequence
- IP address assigned immediately
- Device becomes operational quickly

**Expected Log Sequence**:
```
I (xxxx) opener_main: After NV load select_acd=0
I (xxxx) opener_main: ACD disabled - setting static IP immediately
```

**Verification**:
- Check TCP/IP Interface Object Attribute #1 (`status`): IP should be configured
- Ping the device: Should succeed immediately
- No ACD-related log messages

---

### Test Scenario 4: Runtime ACD Enable/Disable

**Objective**: Verify that ACD can be enabled/disabled at runtime via CIP.

**Steps**:
1. Start device with ACD disabled (`select_acd=0`)
2. Verify IP is assigned
3. Use CIP SetAttributeSingle to set Attribute #10 (`select_acd`) to `1`
4. Change static IP address via CIP
5. Monitor for ACD behavior

**Expected Behavior**:
- When ACD enabled and IP changed: ACD probe runs before new IP assignment
- Configuration persists to NVS

**CIP Request Example** (SetAttributeSingle):
```
Service: SetAttributeSingle (0x10)
Class: 0xF5 (TCP/IP Interface)
Instance: 1
Attribute: 10 (select_acd)
Data: 0x01 (enable ACD)
```

---

### Test Scenario 5: ACD with Network Link Down

**Objective**: Verify ACD behavior when network link is down.

**Steps**:
1. Configure ESP32 with static IP and ACD enabled
2. Disconnect Ethernet cable
3. Power cycle ESP32
4. Monitor serial output
5. Reconnect Ethernet cable

**Expected Behavior**:
- ACD deferred until link is up
- Once link is up, ACD probe sequence runs
- IP assigned after successful ACD

**Expected Log Sequence** (after link up):
```
I (xxxx) opener_main: ACD deferred until link is up
I (xxxx) opener_main: ACD deferred until MAC address is available
I (xxxx) opener_main: RFC 5227: ACD started, IP assignment deferred
```

---

## Verification Methods

### Method 1: Serial Log Analysis

Monitor serial output for ACD-related log messages. Key tags:
- `opener_main`: Main application ACD logic
- `NvTcpip`: NVS load/store operations
- `ACD_DEBUG`: LWIP ACD debug messages (if enabled)

### Method 2: CIP Attribute Reading

Use EtherNet/IP client to read TCP/IP Interface Object attributes:

**Attribute #1** (`status` - DWORD):
- Bit 0: IP configured
- Bit 1: ACD status
- Bit 2: ACD fault

**Attribute #10** (`select_acd` - BOOL):
- `0` = ACD disabled
- `1` = ACD enabled

**Attribute #11** (`last_acd_activity` - BOOL):
- `0` = No conflict
- `1` = Conflict detected (restart)
- `2` = ACD in progress
- `3` = Conflict detected (decline)

### Method 3: Network Tools

**ARP Monitoring**:
```bash
# Linux/Mac
sudo tcpdump -i eth0 arp

# Windows
arp -a
```

**Ping Test**:
```bash
ping 192.168.1.100
```

**Wireshark Capture**:
- Filter: `arp`
- Look for ARP probe packets (sender IP = 0.0.0.0, target IP = test IP)

### Method 4: NVS Inspection

Use ESP-IDF NVS utilities to inspect stored configuration:

```bash
# Dump NVS partition
idf.py partition-table
idf.py flash
idf.py monitor

# Or use nvs_partition_gen.py to inspect
```

---

## Expected Log Messages

### ACD Enabled - Successful Assignment

```
I (xxxx) NvTcpip: Restored TCP/IP configuration (method=Static)
I (xxxx) opener_main: After NV load select_acd=1
I (xxxx) opener_main: ACD path: select_acd=1, RFC5227=1, lwip_netif=0x3ffb1234
I (xxxx) opener_main: Using RFC 5227 compliant ACD for static IP
I (xxxx) opener_main: RFC 5227: ACD started, IP assignment deferred
I (xxxx) opener_main: ACD client registered
I (xxxx) opener_main: Starting ACD probe via acd_start()
I (xxxx) opener_main: acd_start() returned err=0
I (xxxx) opener_main: ACD callback state=0
I (xxxx) opener_main: RFC 5227: IP assigned after ACD confirmation
```

### ACD Enabled - Conflict Detected

```
I (xxxx) opener_main: After NV load select_acd=1
I (xxxx) opener_main: RFC 5227: ACD started, IP assignment deferred
I (xxxx) opener_main: ACD client registered
I (xxxx) opener_main: Starting ACD probe via acd_start()
I (xxxx) opener_main: ACD callback state=1
W (xxxx) opener_main: RFC 5227: IP not assigned due to conflict
```

### ACD Disabled

```
I (xxxx) opener_main: After NV load select_acd=0
I (xxxx) opener_main: ACD disabled - setting static IP immediately
```

---

## Troubleshooting

### Issue: ACD Not Running

**Symptoms**: No ACD log messages, IP assigned immediately

**Possible Causes**:
1. `select_acd` is `0` in NVS
2. `LWIP_ACD` not enabled in build
3. Network link not up

**Solutions**:
- Check NVS: `I (xxxx) opener_main: After NV load select_acd=X`
- Verify build config: `grep LWIP_ACD build/config/sdkconfig.h`
- Check link status: Look for "link is up" messages

### Issue: ACD Always Detects Conflict

**Symptoms**: ACD always reports conflict even when IP is free

**Possible Causes**:
1. Self-conflict (ESP32 seeing its own ARP probes)
2. Network loopback/reflection
3. Another device actually using the IP

**Solutions**:
- Verify MAC address filtering in `acd.c` (should ignore own MAC)
- Use Wireshark to verify ARP traffic
- Test on isolated network segment

### Issue: IP Never Assigned

**Symptoms**: ACD runs but IP never gets assigned

**Possible Causes**:
1. ACD callback not being called
2. `ACD_IP_OK` state not reached
3. Network conditions preventing ACD completion

**Solutions**:
- Enable `ACD_DEBUG` in LWIP for detailed state machine logs
- Check for `ACD callback state=0` message
- Verify network connectivity and ARP functionality

### Issue: Configuration Not Persisting

**Symptoms**: ACD setting resets after reboot

**Possible Causes**:
1. NVS write failure
2. NVS partition full
3. Configuration not being saved

**Solutions**:
- Check for `NvTcpipStore` errors in logs
- Verify NVS partition size in partition table
- Use CIP SetAttributeSingle to ensure `kNvDataFunc` flag is set

### Debugging Tips

1. **Enable ACD Debug Logging**:
   Add to `sdkconfig`:
   ```
   CONFIG_LWIP_DEBUG=y
   CONFIG_LWIP_ACD_DEBUG=y
   ```

2. **Monitor ARP Traffic**:
   Use Wireshark with filter: `arp and arp.dst.proto_ipv4 == 192.168.1.100`

3. **Check ACD State Machine**:
   Look for state transitions in logs:
   - `ACD_STATE_PROBE_WAIT`
   - `ACD_STATE_PROBING`
   - `ACD_STATE_ANNOUNCE_WAIT`
   - `ACD_STATE_ANNOUNCING`
   - `ACD_STATE_ONGOING`

4. **Verify NVS Contents**:
   Use ESP-IDF NVS utilities or custom dump tool to inspect stored configuration

---

## Additional Resources

- **RFC 5227**: [IPv4 Address Conflict Detection](https://tools.ietf.org/html/rfc5227) - Base ACD specification
- **ODVA Pub 28**: [Recommended IP Addressing Methods for EtherNet/IP™ Devices](https://www.odva.org/technology-standards/document-library/) - ODVA recommendations for ACD in EtherNet/IP devices
- **LWIP ACD Documentation**: `dependency_modifications/lwIP/acd-static-ip-issue.md`
- **RFC 5227 Implementation Guide**: `dependency_modifications/lwIP/RFC5227_IMPLEMENTATION_GUIDE.md`
- **EtherNet/IP References**: `docs/EtherNetIP_References.md` - Comprehensive ODVA publication guide
- **OpENer Documentation**: See main `README.md`

---

## Test Checklist

- [ ] Test Scenario 1: ACD enabled, no conflict (success)
- [ ] Test Scenario 2: ACD enabled, conflict detected
- [ ] Test Scenario 3: ACD disabled, immediate assignment
- [ ] Test Scenario 4: Runtime ACD enable/disable
- [ ] Test Scenario 5: ACD with link down/up
- [ ] Verify NVS persistence across reboots
- [ ] Verify CIP attribute reading/writing
- [ ] Verify network connectivity after ACD success
- [ ] Verify no IP assignment after conflict detection

---

**Last Updated**: 2024
**Project**: ESP32-P4 OpENer EtherNet/IP Stack

