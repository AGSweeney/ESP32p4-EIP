# EtherNet/IP Adapter Protocol Reference

> **Source**: Extracted from Hilscher EtherNet/IP Adapter Protocol API Documentation (V2.13.0)  
> **Note**: This document focuses on adapter-specific protocol details and implementation guidance for EtherNet/IP adapter devices.

## Table of Contents

1. [Adapter Overview](#adapter-overview)
2. [CIP Object Details for Adapters](#cip-object-details-for-adapters)
3. [Connection Management (Adapter Perspective)](#connection-management-adapter-perspective)
4. [I/O Data Handling](#io-data-handling)
5. [Adapter Responsibilities](#adapter-responsibilities)
6. [Status and Error Handling](#status-and-error-handling)
7. [Configuration and Initialization](#configuration-and-initialization)

---

## Adapter Overview

### Adapter Role in EtherNet/IP

An EtherNet/IP **Adapter** (also called a **Target** or **Connection Target**) is a device that:

- **Accepts connections** from scanners/controllers (originators)
- **Responds to explicit messages** for configuration and diagnostics
- **Exchanges I/O data** via implicit connections
- **Implements CIP objects** to expose device capabilities
- **Manages connection state** and handles timeouts

### Key Differences: Adapter vs Scanner

| Aspect | Adapter (Target) | Scanner (Originator) |
|--------|------------------|----------------------|
| Connection Initiation | Receives Forward_Open | Sends Forward_Open |
| I/O Data Direction | Produces inputs, consumes outputs | Consumes inputs, produces outputs |
| Connection Control | Passive (accepts connections) | Active (establishes connections) |
| Primary Role | Device/endpoint | Controller/master |

---

## CIP Object Details for Adapters

### Identity Object (Class Code: 0x01)

**Purpose**: Device identification and status reporting.

#### Instance Attributes (Instance 1)

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Vendor ID | UINT | R | ODVA-assigned vendor identifier |
| 2 | Device Type | UINT | R | Device type code |
| 3 | Product Code | UINT | R | Vendor-specific product code |
| 4 | Revision | STRUCT | R | Major.Minor revision (USINT, USINT) |
| 5 | Status | WORD | R | Device status flags |
| 6 | Serial Number | UDINT | R | Unique device serial number |
| 7 | Product Name | STRING | R | Human-readable product name |

#### Status Word (Attribute 5) - Adapter Behavior

The adapter must maintain accurate status flags:

- **Bit 0 (Owned)**: Set when scanner establishes exclusive owner connection
- **Bit 6 (Minor Recoverable Fault)**: Set for recoverable faults
- **Bit 7 (Minor Unrecoverable Fault)**: Set for minor unrecoverable faults
- **Bit 8 (Major Recoverable Fault)**: Set for major recoverable faults
- **Bit 9 (Major Unrecoverable Fault)**: Set for fatal faults

#### Supported Services

- **Get_Attribute_All (0x01)**: Scanner queries device identity
- **Get_Attribute_Single (0x0E)**: Scanner reads specific attribute
- **Set_Attribute_Single (0x10)**: Limited - only certain attributes settable
- **Reset (0x05)**: 
  - **Type 0**: Reset device (may require reconfiguration)
  - **Type 1**: Reset and maintain configuration

#### Adapter Responsibilities

- Maintain accurate device information
- Update status flags based on device state
- Respond to reset requests appropriately
- Handle state transitions (Startup → Standby → Operational)

### Assembly Object (Class Code: 0x04)

**Purpose**: Groups I/O data for implicit connections.

#### Common Assembly Instances

| Instance ID | Name | Direction | Description |
|-------------|------|-----------|-------------|
| 100 (0x64) | Input Assembly | Produced (T→O) | Data sent to scanner |
| 150 (0x96) | Output Assembly | Consumed (O→T) | Data received from scanner |
| 151 (0x97) | Configuration Assembly | Consumed (O→T) | Configuration data |

#### Instance Attributes

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Data | ARRAY of BYTE | R/W | Assembly data content |
| 2 | Data Size | UINT | R | Size of data array in bytes |
| 3 | Connection Point List | ARRAY | R | List of connections using this assembly |

#### Adapter Responsibilities

- **Input Assembly (100)**:
  - Update data cyclically at RPI rate
  - Increment sequence counter on data change
  - Send data to scanner via UDP implicit connection

- **Output Assembly (150)**:
  - Receive data from scanner
  - Process output data (e.g., update physical outputs)
  - Validate data integrity

- **Configuration Assembly (151)**:
  - Receive configuration data during Forward_Open
  - Apply configuration to device
  - Validate configuration parameters

#### Data Update Behavior

- **Produced Data (Input Assembly)**:
  - Updated by application at regular intervals
  - Sequence counter increments when data changes
  - Sent to scanner at RPI rate
  - May include Run/Idle header if configured

- **Consumed Data (Output Assembly)**:
  - Received from scanner at RPI rate
  - Processed immediately upon receipt
  - Validated before applying to outputs

### Connection Manager Object (Class Code: 0x06)

**Purpose**: Manages explicit and implicit connections.

#### Adapter Connection Handling

The adapter receives and processes connection requests:

1. **Forward_Open Request**:
   - Validates connection parameters
   - Checks resource availability
   - Establishes connection if valid
   - Returns Forward_Open response

2. **Forward_Close Request**:
   - Closes specified connection
   - Releases resources
   - Returns Forward_Close response

3. **Connection Monitoring**:
   - Monitors connection timeouts
   - Handles missing data packets
   - Manages connection state transitions

#### Connection Types Handled by Adapter

| Connection Type | Description | Adapter Behavior |
|----------------|-------------|-----------------|
| Exclusive Owner | Scanner has exclusive control | Accept only one connection, reject others |
| Input Only | Scanner reads data only | Accept multiple connections, produce data |
| Listen Only | Scanner reads but doesn't control | Accept multiple connections, produce data |

#### Connection State Machine (Adapter)

```
Non-existent → Configuring → Established → Closing → Non-existent
                    ↓              ↓
                 Timed Out    Deferred Delete
```

**State Transitions**:
- **Non-existent → Configuring**: Forward_Open received
- **Configuring → Established**: Connection validated and opened
- **Established → Timed Out**: No data received within timeout period
- **Established → Closing**: Forward_Close received
- **Closing → Non-existent**: Resources released

### TCP/IP Interface Object (Class Code: 0xF5)

**Purpose**: Network configuration and status.

#### Key Attributes for Adapters

| Attribute ID | Name | Description |
|--------------|------|-------------|
| 3 | Configuration Control | Active configuration method (Static/DHCP/BOOTP) |
| 5 | Interface Configuration | IP address, subnet mask, gateway |
| 10 | Select ACD | Enable Address Conflict Detection |
| 11 | ACD Timeout | ACD timeout value |
| 12 | ACD Count | Number of conflicts detected |
| 13 | Encapsulation Inactivity Timeout | Timeout for encapsulation inactivity |

#### Adapter Responsibilities

- Maintain network configuration
- Support static IP and DHCP
- Implement Address Conflict Detection (ACD) per RFC 5227
- Report network status
- Handle configuration changes

### Ethernet Link Object (Class Code: 0xF6)

**Purpose**: Physical layer information and statistics.

#### Key Attributes

| Attribute ID | Name | Description |
|--------------|------|-------------|
| 1 | Interface Speed | Link speed (10/100/1000 Mbps) |
| 2 | Interface Flags | Capabilities and status flags |
| 3 | Physical Address | MAC address |
| 4 | Interface Counters | Receive/transmit statistics |
| 5 | Media Counters | Physical layer statistics |
| 7 | Interface State | Current interface state |
| 8 | Admin State | Administrative control |

#### Adapter Responsibilities

- Report accurate link status
- Maintain interface counters
- Support Get_And_Clear service for counter reset
- Update state based on physical link status

### Quality of Service Object (Class Code: 0x48)

**Purpose**: Configures DSCP priorities for network traffic.

#### DSCP Values (Default)

| Priority | DSCP Value | Usage |
|----------|------------|-------|
| Urgent | 55 | Critical real-time I/O |
| Scheduled | 47 | Scheduled I/O updates |
| High | 43 | High priority data |
| Low | 31 | Normal priority data |
| Explicit | 27 | Configuration messages |

#### Adapter Responsibilities

- Apply DSCP values to outgoing packets
- Support 802.1Q VLAN tagging if enabled
- Configure PTP DSCP override if needed
- Maintain QoS settings

---

## Connection Management (Adapter Perspective)

### Forward_Open Processing

When an adapter receives a Forward_Open request, it must:

1. **Parse Request**:
   - Extract connection parameters
   - Validate connection path
   - Check connection type

2. **Validate Parameters**:
   - Connection IDs (O→T, T→O)
   - Connection Serial Number
   - RPI values
   - Connection size
   - Transport class

3. **Check Resources**:
   - Available connection slots
   - Memory for connection data
   - Assembly instances exist
   - Connection type compatibility

4. **Establish Connection**:
   - Create connection object
   - Set up UDP sockets for implicit I/O
   - Initialize connection state
   - Start connection timers

5. **Send Response**:
   - Success: Return connection IDs and parameters
   - Failure: Return error code and extended status

### Forward_Open Request Structure

Key fields in Forward_Open request:

```
- Connection Path (to target assembly)
- O→T Connection ID (Originator to Target)
- T→O Connection ID (Target to Originator)
- Connection Serial Number
- Vendor ID
- Originator Serial Number
- Connection Timeout Multiplier
- O→T RPI (Requested Packet Interval)
- T→O RPI
- Transport Class
- Connection Type
- Connection Priority
- Connection Size (O→T and T→O)
- Configuration Data (optional)
```

### Forward_Open Response Structure

Adapter response includes:

```
- O→T Connection ID (confirmed)
- T→O Connection ID (confirmed)
- Connection Serial Number (echoed)
- Vendor ID (echoed)
- Originator Serial Number (echoed)
- Application Reply Size
- Reserved
```

### Forward_Close Processing

When adapter receives Forward_Close:

1. **Validate Request**:
   - Connection Serial Number matches
   - Vendor ID matches
   - Originator Serial Number matches

2. **Close Connection**:
   - Stop data transfer
   - Close UDP sockets
   - Release resources
   - Update connection state

3. **Send Response**:
   - Confirm closure
   - Return status

### Connection Timeout Handling

**Timeout Calculation**:
```
Timeout = RPI × Connection Timeout Multiplier
```

**Timeout Multipliers**:
- **0**: 4× RPI
- **1**: 8× RPI
- **2**: 16× RPI
- **3**: 32× RPI

**Adapter Behavior on Timeout**:
- Stop producing data
- Mark connection as timed out
- May transition to fault state
- Wait for Forward_Close or reconnection

### Connection Point Types

| Type | Description | Adapter Behavior |
|------|-------------|-----------------|
| Exclusive Owner | Single scanner control | Reject additional connections to same assembly |
| Input Only | Read-only connection | Allow multiple connections |
| Listen Only | Read-only, no control | Allow multiple connections |

---

## I/O Data Handling

### Implicit Messaging Structure

#### Data Packet Format

```
[EtherNet/IP Encapsulation Header]
[Connection Header] (optional)
[I/O Data]
```

#### Connection Header (Optional)

If Run/Idle header enabled:

```
Offset  Length  Field
0       2       Sequence Count (UINT)
2       1       Run/Idle Status (BOOL)
3       1       Reserved
```

### Produced Data (Input Assembly)

**Adapter Responsibilities**:

1. **Data Update**:
   - Application updates input assembly data
   - Sequence counter increments on change
   - Data prepared for transmission

2. **Transmission**:
   - Sent at RPI interval
   - UDP packet to scanner
   - Includes connection header if configured
   - Uses connection-specific socket

3. **Sequence Counter**:
   - Increments when data changes
   - Helps scanner detect updates
   - Wraps around at maximum value

### Consumed Data (Output Assembly)

**Adapter Responsibilities**:

1. **Reception**:
   - Receive UDP packets from scanner
   - Validate connection ID
   - Extract I/O data

2. **Processing**:
   - Validate data integrity
   - Update output assembly attribute
   - Apply to physical outputs
   - Update sequence counter if needed

3. **Error Handling**:
   - Handle missing packets
   - Detect connection timeout
   - Validate data size

### RPI (Requested Packet Interval)

**Purpose**: Defines cyclic update rate for implicit connections.

**Typical Values**:
- **1ms - 10ms**: Fast I/O (motion control, high-speed I/O)
- **10ms - 100ms**: Standard I/O (discrete I/O, analog I/O)
- **100ms - 1000ms**: Slow updates (status, diagnostics)

**Adapter Requirements**:
- Must meet RPI requirements
- Data must be ready before transmission time
- Jitter should be minimized
- Missing updates may cause timeout

### Data Change Detection

**Sequence Counter Behavior**:
- Increments when data changes
- Helps scanner detect updates
- Reduces unnecessary processing
- Wraps at maximum value (65535)

**Adapter Implementation**:
- Compare current data with previous
- Increment counter on change
- Include counter in transmission
- Reset on connection establishment

---

## Adapter Responsibilities

### Core Responsibilities

1. **Connection Management**:
   - Accept Forward_Open requests
   - Validate connection parameters
   - Establish connections
   - Handle Forward_Close requests
   - Monitor connection timeouts

2. **I/O Data Exchange**:
   - Produce input data at RPI rate
   - Consume output data from scanner
   - Maintain sequence counters
   - Handle data validation

3. **Explicit Messaging**:
   - Process CIP service requests
   - Return appropriate responses
   - Handle error conditions
   - Support required services

4. **Object Management**:
   - Maintain object attributes
   - Update status flags
   - Handle state transitions
   - Support required services

5. **Error Handling**:
   - Detect and report errors
   - Return appropriate error codes
   - Handle fault conditions
   - Maintain error state

### State Management

**Device States**:
- **Startup**: Initial power-on state
- **Standby**: Ready but not operational
- **Operational**: Normal operation
- **Faulted**: Error condition

**State Transitions**:
- Startup → Standby: Initialization complete
- Standby → Operational: Scanner establishes connection
- Operational → Standby: Connection closed
- Any → Faulted: Error detected
- Faulted → Standby: Error cleared

### Watchdog Handling

**Purpose**: Detect connection failures and timeouts.

**Adapter Behavior**:
- Monitor connection activity
- Detect missing data packets
- Trigger timeout on inactivity
- Update connection state
- May transition to fault state

**Watchdog Clearing**:
- Scanner may send explicit clear request
- Adapter clears watchdog error
- Connection may resume
- State returns to operational

---

## Status and Error Handling

### Status Reporting

**Identity Object Status**:
- Maintains device status word
- Updates flags based on state
- Reports faults and errors
- Indicates connection ownership

**Connection Status**:
- Tracks connection state
- Reports connection errors
- Monitors connection health
- Handles timeout conditions

### Error Codes

#### General CIP Errors

| Code | Name | Adapter Behavior |
|------|------|-----------------|
| 0x00 | Success | Operation completed successfully |
| 0x01 | Connection Failure | Connection cannot be established |
| 0x02 | Resource Unavailable | Required resource not available |
| 0x03 | Invalid Parameter Value | Parameter value out of range |
| 0x08 | Service Not Supported | Service not implemented |
| 0x0E | Attribute Not Settable | Attribute is read-only |
| 0x10 | Device State Conflict | Device in wrong state for operation |

#### Connection Manager Errors

| Code | Name | Description |
|------|------|-------------|
| 0x0100 | Connection In Use | Connection already exists |
| 0x0101 | Transport Not Supported | Transport class not supported |
| 0x0102 | Owner Conflict | Exclusive owner connection conflict |
| 0x0103 | Connection Not Found | Connection does not exist |
| 0x0104 | Invalid Connection Type | Connection type invalid |
| 0x0105 | Invalid Connection Size | Connection size mismatch |
| 0x0106 | Module Not Configured | Required module not configured |
| 0x0108 | Connection Rejected | Connection rejected by adapter |

### Error Handling Best Practices

1. **Validate All Inputs**:
   - Check parameter ranges
   - Validate connection paths
   - Verify data sizes

2. **Return Appropriate Errors**:
   - Use correct error codes
   - Include extended status when available
   - Provide meaningful error information

3. **Handle Timeouts Gracefully**:
   - Detect timeout conditions
   - Update connection state
   - Release resources
   - Log timeout events

4. **Maintain State Consistency**:
   - Update state atomically
   - Handle state transitions correctly
   - Prevent invalid state combinations

---

## Configuration and Initialization

### Initialization Sequence

1. **Hardware Initialization**:
   - Initialize Ethernet interface
   - Configure MAC address
   - Set up network interface

2. **Network Configuration**:
   - Configure IP address (static or DHCP)
   - Set subnet mask and gateway
   - Enable Address Conflict Detection if static

3. **CIP Stack Initialization**:
   - Initialize CIP objects
   - Register assembly instances
   - Set up connection manager
   - Configure default attributes

4. **Application Initialization**:
   - Initialize I/O drivers
   - Set up data structures
   - Configure application-specific objects
   - Start background tasks

5. **Start Listening**:
   - Open TCP port 44818 (explicit)
   - Open UDP port 44818 (explicit unconnected)
   - Ready to accept connections

### Configuration Persistence

**Non-Volatile Storage**:
- Store network configuration
- Save device identity attributes
- Persist assembly configurations
- Maintain configuration across reboots

**Configuration Sources**:
- Non-volatile memory (NVS)
- Configuration files
- CIP object attributes
- Explicit messages from scanner

### Assembly Registration

**Process**:
1. Create assembly instance
2. Set data size
3. Initialize data buffer
4. Register with connection manager
5. Make available for connections

**Required Assemblies**:
- Input Assembly (100) - Produced data
- Output Assembly (150) - Consumed data
- Configuration Assembly (151) - Optional

---

## Implementation Notes

### Best Practices

1. **Connection Handling**:
   - Validate all connection parameters
   - Check resource availability
   - Handle connection timeouts gracefully
   - Support multiple connection types

2. **I/O Data Processing**:
   - Meet RPI requirements
   - Minimize processing latency
   - Validate data integrity
   - Handle missing packets

3. **Error Reporting**:
   - Return accurate error codes
   - Log errors for debugging
   - Update status flags appropriately
   - Provide diagnostic information

4. **Performance**:
   - Optimize data transfer
   - Minimize CPU usage
   - Efficient memory usage
   - Fast error recovery

### Common Pitfalls

1. **Connection Timeouts**:
   - Not monitoring connection activity
   - Incorrect timeout calculation
   - Not handling timeout gracefully

2. **Data Synchronization**:
   - Not updating sequence counters
   - Missing data updates
   - Incorrect RPI handling

3. **State Management**:
   - Invalid state transitions
   - Not updating status flags
   - Inconsistent state

4. **Resource Management**:
   - Not releasing resources on close
   - Memory leaks
   - Socket management errors

---

## References

- **Hilscher EtherNet/IP Adapter Protocol API V2.13.0**: Primary source document
- **ODVA EtherNet/IP Specification**: Official protocol specification
- **CIP Specification**: Common Industrial Protocol specification
- **OpENer Stack**: Open-source EtherNet/IP implementation reference

---

*Last Updated: Based on Hilscher EtherNet/IP Adapter Protocol API V2.13.0*  
*Project: ESP32-P4 EtherNet/IP Adapter Implementation*

