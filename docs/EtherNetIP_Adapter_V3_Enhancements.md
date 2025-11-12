# EtherNet/IP Adapter V3.4.0 Enhancements

> **Source**: Extracted from Hilscher EtherNet/IP Adapter Protocol API V3.4.0 Documentation  
> **Note**: This document highlights new features and enhancements in V3.4.0 compared to V2.13.0, specifically focusing on adapter implementation details.

## Table of Contents

1. [New CIP Objects](#new-cip-objects)
2. [Handshake Modes](#handshake-modes)
3. [Enhanced Connection Management](#enhanced-connection-management)
4. [Additional Application Interface Features](#additional-application-interface-features)

---

## New CIP Objects

### Predefined Connection Object (Class Code: 0x401)

**Purpose**: Manages predefined connections for faster connection establishment and reduced configuration overhead.

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Connection Type | UINT | R/W | Type of predefined connection |
| 2 | Connection Path | EPATH | R/W | Path to target assembly |
| 3 | O→T Connection ID | UDINT | R/W | Originator to Target connection ID |
| 4 | T→O Connection ID | UDINT | R/W | Target to Originator connection ID |
| 5 | RPI | UDINT | R/W | Requested Packet Interval |
| 6 | Connection Timeout Multiplier | USINT | R/W | Timeout multiplier |
| 7 | Connection Size O→T | UINT | R/W | Output data size |
| 8 | Connection Size T→O | UINT | R/W | Input data size |
| 9 | Transport Class | USINT | R/W | Transport class |
| 10 | Connection Priority | USINT | R/W | Connection priority |

#### Supported Services

- **Get_Attribute_All (0x01)**: Retrieve all attributes
- **Get_Attribute_Single (0x0E)**: Retrieve single attribute
- **Set_Attribute_Single (0x10)**: Set single attribute
- **Create (0x08)**: Create predefined connection instance
- **Delete (0x09)**: Delete predefined connection instance

#### Benefits

- **Faster Connection Establishment**: Pre-configured connections reduce setup time
- **Reduced Configuration Overhead**: Connections can be pre-defined and activated quickly
- **Simplified Management**: Centralized connection configuration

#### Adapter Implementation Notes

- Predefined connections can be created during device initialization
- Scanner can use predefined connections for faster I/O setup
- Adapter must validate predefined connection parameters
- Predefined connections still require Forward_Open to activate

### IO Mapping Object (Class Code: 0x402)

**Purpose**: Maps I/O data between physical I/O points and assembly instances.

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Mapping Type | UINT | R/W | Type of I/O mapping |
| 2 | Source Assembly | UINT | R/W | Source assembly instance ID |
| 3 | Destination Assembly | UINT | R/W | Destination assembly instance ID |
| 4 | Mapping Data | ARRAY | R/W | Mapping configuration data |
| 5 | Mapping Size | UINT | R | Size of mapping data |

#### Supported Services

- **Get_Attribute_All (0x01)**: Retrieve all mapping attributes
- **Get_Attribute_Single (0x0E)**: Retrieve single attribute
- **Set_Attribute_Single (0x10)**: Set single attribute
- **Create (0x08)**: Create new mapping instance
- **Delete (0x09)**: Delete mapping instance

#### Benefits

- **Flexible I/O Mapping**: Map physical I/O to different assemblies
- **Dynamic Reconfiguration**: Change I/O mapping without reconfiguring assemblies
- **Modular Design**: Separate I/O mapping from assembly configuration

#### Adapter Implementation Notes

- Useful for devices with configurable I/O mapping
- Allows runtime reconfiguration of I/O assignments
- Can simplify assembly management
- Requires careful validation of mapping data

---

## Handshake Modes

**Purpose**: Synchronize I/O data exchange between adapter and scanner for deterministic, synchronized operation.

### Overview

Handshake modes provide mechanisms to ensure that I/O data is exchanged in a synchronized manner, preventing race conditions and ensuring data consistency.

### Input Handshake Mode / Output Handshake Mode

**Purpose**: Synchronize input and output data exchange separately.

#### Input Handshake Mode

- **Adapter Behavior**: 
  - Waits for handshake signal before updating input assembly
  - Produces input data only after receiving handshake
  - Ensures scanner receives data at expected times

- **Use Cases**:
  - Synchronized input sampling
  - Deterministic input updates
  - Coordinated data acquisition

#### Output Handshake Mode

- **Adapter Behavior**:
  - Consumes output data from scanner
  - Applies outputs to physical I/O
  - Sends handshake acknowledgment after output applied

- **Use Cases**:
  - Synchronized output updates
  - Deterministic output application
  - Coordinated control actions

### Synchronization Handshake Mode

**Purpose**: Synchronize both input and output data exchange together.

#### Behavior

- **Bidirectional Synchronization**:
  - Input and output data exchanged together
  - Scanner and adapter coordinate data exchange
  - Ensures data consistency

- **Synchronization Signal**:
  - Adapter waits for synchronization signal
  - Updates inputs and applies outputs simultaneously
  - Sends acknowledgment after processing

#### Use Cases

- **Motion Control**: Synchronized position feedback and command updates
- **Coordinated Systems**: Multiple devices operating in sync
- **Deterministic Operation**: Precise timing requirements
- **Safety Systems**: Critical synchronized operations

### Configuration

Handshake modes are configured through:

1. **Assembly Attributes**: Handshake mode can be specified per assembly
2. **Connection Parameters**: Handshake mode included in Forward_Open
3. **Device Configuration**: Default handshake mode for device

#### Configuration Parameters

- **Handshake Mode Type**: Input, Output, or Synchronization
- **Handshake Timeout**: Maximum wait time for handshake signal
- **Handshake Signal Source**: Where handshake signal originates
- **Handshake Acknowledgment**: Whether acknowledgment required

### Implementation Considerations

#### Adapter Requirements

1. **Handshake Signal Detection**:
   - Monitor for handshake signals
   - Validate handshake timing
   - Handle handshake timeouts

2. **Data Synchronization**:
   - Coordinate input and output updates
   - Ensure data consistency
   - Handle synchronization failures

3. **Performance**:
   - Minimize handshake overhead
   - Optimize synchronization timing
   - Balance determinism with throughput

#### Benefits

- **Deterministic Operation**: Predictable I/O timing
- **Data Consistency**: Synchronized updates prevent race conditions
- **Coordinated Control**: Multiple devices can operate in sync
- **Safety**: Critical for safety-critical applications

#### Trade-offs

- **Latency**: Handshake adds some latency to I/O updates
- **Complexity**: More complex than simple cyclic I/O
- **Overhead**: Additional protocol overhead for handshake signals

---

## Enhanced Connection Management

### Forward_Open Indication

**Purpose**: Notify application when Forward_Open request is received.

#### Indication Details

- **Trigger**: Forward_Open request received from scanner
- **Information Provided**:
  - Connection parameters
  - Assembly instance IDs
  - Connection IDs
  - RPI values
  - Connection type

#### Adapter Behavior

1. **Receive Forward_Open**:
   - Parse connection parameters
   - Validate request
   - Check resource availability

2. **Send Indication**:
   - Notify application of Forward_Open request
   - Provide connection details
   - Wait for application response

3. **Process Response**:
   - Application can accept or reject connection
   - Adapter processes application decision
   - Send Forward_Open response to scanner

#### Use Cases

- **Application Validation**: Application can validate connection before accepting
- **Resource Management**: Application can check resource availability
- **Custom Logic**: Application can implement custom connection logic
- **Logging**: Log connection establishment attempts

### Forward_Open_Completion Indication

**Purpose**: Notify application when Forward_Open completes (success or failure).

#### Indication Details

- **Trigger**: Forward_Open response sent to scanner
- **Information Provided**:
  - Connection status (success/failure)
  - Connection IDs (if successful)
  - Error code (if failed)
  - Extended status (if applicable)

#### Adapter Behavior

1. **Complete Forward_Open**:
   - Process Forward_Open request
   - Establish connection (if successful)
   - Send response to scanner

2. **Send Completion Indication**:
   - Notify application of completion
   - Provide connection status
   - Include error information if failed

#### Use Cases

- **Connection Tracking**: Track active connections
- **Error Handling**: Handle connection failures
- **Resource Cleanup**: Clean up resources on failure
- **Status Reporting**: Report connection status

### Forward_Close Indication

**Purpose**: Notify application when Forward_Close request is received.

#### Indication Details

- **Trigger**: Forward_Close request received from scanner
- **Information Provided**:
  - Connection Serial Number
  - Vendor ID
  - Originator Serial Number

#### Adapter Behavior

1. **Receive Forward_Close**:
   - Parse close request
   - Validate connection
   - Find connection to close

2. **Send Indication**:
   - Notify application of close request
   - Provide connection details
   - Wait for application acknowledgment

3. **Close Connection**:
   - Release connection resources
   - Close UDP sockets
   - Update connection state
   - Send Forward_Close response

#### Use Cases

- **Resource Cleanup**: Application can clean up resources
- **State Management**: Update application state
- **Logging**: Log connection closures
- **Graceful Shutdown**: Handle connection closure gracefully

---

## Additional Application Interface Features

### Enhanced Configuration Options

#### Configuration Sets

V3.4.0 provides multiple configuration sets:

1. **Basic Configuration Set**:
   - Minimal configuration packets
   - Simple configuration process
   - Suitable for basic devices

2. **Extended Configuration Set**:
   - Additional configuration options
   - More flexible configuration
   - Enhanced features

#### Configuration Sequence

- **Step-by-step Process**: Clear configuration sequence
- **Validation**: Configuration validation at each step
- **Error Handling**: Comprehensive error handling
- **Rollback**: Ability to rollback configuration changes

### Additional Services

#### Get Module Status/Network Status

- **Purpose**: Retrieve current module and network status
- **Use Cases**: Diagnostics, status monitoring, troubleshooting

#### Get Watchdog Time

- **Purpose**: Retrieve current watchdog timeout value
- **Use Cases**: Monitoring, diagnostics, configuration verification

#### Get DPM I/O Information

- **Purpose**: Retrieve Dual Port Memory I/O information
- **Use Cases**: I/O configuration, diagnostics, monitoring

#### Lock/Unlock Configuration

- **Purpose**: Prevent configuration changes during operation
- **Use Cases**: Safety, preventing accidental changes, critical operations

#### Get Firmware Identification

- **Purpose**: Retrieve firmware version and identification
- **Use Cases**: Version checking, compatibility verification, diagnostics

#### Get Firmware Parameter

- **Purpose**: Retrieve firmware-specific parameters
- **Use Cases**: Configuration, diagnostics, advanced features

### Enhanced Error Handling

#### Error Code Extensions

- **More Detailed Error Codes**: Additional error codes for specific conditions
- **Extended Status**: More detailed status information
- **Error Context**: Additional context for error conditions

#### Error Reporting

- **Structured Error Information**: Consistent error reporting format
- **Error Logging**: Enhanced error logging capabilities
- **Error Recovery**: Improved error recovery mechanisms

---

## Implementation Recommendations

### For ESP32-P4 OpENer Project

#### Handshake Modes

1. **Evaluate Need**: Determine if handshake modes are required for your application
2. **Implementation Priority**: Consider handshake modes for Phase 2 (I/O Abstraction Layer)
3. **Performance Impact**: Assess performance impact of handshake overhead
4. **Use Cases**: Identify specific use cases that benefit from handshake modes

#### Predefined Connection Object

1. **Connection Management**: Consider predefined connections for common I/O configurations
2. **Startup Optimization**: Use predefined connections to reduce startup time
3. **Configuration**: Pre-configure common connection patterns

#### IO Mapping Object

1. **I/O Flexibility**: Evaluate if IO Mapping Object adds value for your I/O abstraction
2. **Dynamic Reconfiguration**: Consider if runtime I/O remapping is needed
3. **Complexity**: Balance flexibility with implementation complexity

#### Enhanced Connection Management

1. **Indication Handling**: Implement Forward_Open/Forward_Close indications if needed
2. **Application Integration**: Integrate connection indications with application logic
3. **Error Handling**: Use enhanced error handling for better diagnostics

### Priority Assessment

| Feature | Priority | Effort | Benefit | Recommendation |
|---------|----------|--------|---------|---------------|
| Handshake Modes | Medium | High | Medium | Consider for Phase 2+ |
| Predefined Connection Object | Low | Medium | Low | Optional enhancement |
| IO Mapping Object | Medium | Medium | Medium | Consider for I/O abstraction |
| Enhanced Connection Management | High | Low | High | Implement for better diagnostics |

---

## Comparison: V2.13.0 vs V3.4.0

### New Features in V3.4.0

| Feature | V2.13.0 | V3.4.0 | Impact |
|--------|---------|--------|--------|
| Handshake Modes | ❌ | ✅ | High - Synchronization support |
| Predefined Connection Object | ❌ | ✅ | Medium - Faster connections |
| IO Mapping Object | ❌ | ✅ | Medium - Flexible I/O mapping |
| Forward_Open Indication | ❌ | ✅ | Medium - Better integration |
| Forward_Close Indication | ❌ | ✅ | Medium - Better cleanup |
| Enhanced Error Handling | Basic | Enhanced | High - Better diagnostics |
| Additional Services | Limited | Extended | Medium - More features |

### Key Improvements

1. **Synchronization**: Handshake modes provide deterministic I/O synchronization
2. **Flexibility**: New objects provide more configuration flexibility
3. **Integration**: Enhanced indications improve application integration
4. **Diagnostics**: Better error handling and status reporting

---

## References

- **Hilscher EtherNet/IP Adapter Protocol API V3.4.0**: [Source Document](https://www.hilscher.com/fileadmin/cms_upload/de/Resources/pdf/EtherNetIP_Adapter_V3_Protocol_API_04_EN.pdf)
- **Hilscher EtherNet/IP Adapter Protocol API V2.13.0**: Previous version reference
- **EtherNet/IP Adapter Protocol Reference**: See [docs/EtherNetIP_Adapter_Protocol_Reference.md](../docs/EtherNetIP_Adapter_Protocol_Reference.md)

---

*Last Updated: Based on Hilscher EtherNet/IP Adapter Protocol API V3.4.0*  
*Project: ESP32-P4 EtherNet/IP Adapter Implementation*

