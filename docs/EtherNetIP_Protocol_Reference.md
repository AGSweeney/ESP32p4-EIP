# EtherNet/IP Protocol Reference

> **Source**: Extracted from Hilscher EtherNet/IP Scanner Protocol API Documentation (V2.10.0)  
> **Note**: This document extracts protocol-level information that is useful for adapter implementations, even though the source document targets scanner implementations. Protocol details are generally applicable to both roles.

## Table of Contents

1. [CIP Fundamentals](#cip-fundamentals)
2. [CIP Object Reference](#cip-object-reference)
3. [Connection Management](#connection-management)
4. [Messaging Types](#messaging-types)
5. [Error Codes and Status](#error-codes-and-status)
6. [Protocol Details](#protocol-details)

---

## CIP Fundamentals

### Common Industrial Protocol (CIP)

CIP is the application-layer protocol used by EtherNet/IP. It provides an object-oriented model for industrial device communication.

#### Key Concepts

- **Object Model**: Devices are modeled as collections of objects
- **Classes**: Templates that define object structure (e.g., Identity Object Class 0x01)
- **Instances**: Specific occurrences of a class (e.g., Identity Object Instance 1)
- **Attributes**: Data elements within objects (e.g., Vendor ID, Product Code)
- **Services**: Operations that can be performed on objects (e.g., Get_Attribute_Single)

#### Addressing Schema

CIP uses a hierarchical addressing scheme:
- **Class Code**: Identifies the object class (e.g., 0x01 = Identity)
- **Instance ID**: Identifies a specific instance (e.g., 1 = first instance)
- **Attribute ID**: Identifies an attribute within an instance (e.g., 1 = Vendor ID)

**Example**: `0x01/1/1` = Identity Object, Instance 1, Attribute 1 (Vendor ID)

### CIP Data Types

Common CIP data types used in EtherNet/IP:

| Type Code | Name | Size (bytes) | Description |
|-----------|------|--------------|-------------|
| 0xC1 | BOOL | 1 | Boolean (0 or 1) |
| 0xC2 | SINT | 1 | Signed 8-bit integer |
| 0xC3 | INT | 2 | Signed 16-bit integer |
| 0xC4 | DINT | 4 | Signed 32-bit integer |
| 0xC5 | LINT | 8 | Signed 64-bit integer |
| 0xC6 | USINT | 1 | Unsigned 8-bit integer |
| 0xC7 | UINT | 2 | Unsigned 16-bit integer |
| 0xC8 | UDINT | 4 | Unsigned 32-bit integer |
| 0xC9 | ULINT | 8 | Unsigned 64-bit integer |
| 0xCA | REAL | 4 | IEEE 754 single precision |
| 0xCB | LREAL | 8 | IEEE 754 double precision |
| 0xDA | STRING | Variable | Character string |
| 0xE0 | BYTE | 1 | 8-bit bit string |
| 0xE1 | WORD | 2 | 16-bit bit string |
| 0xE2 | DWORD | 4 | 32-bit bit string |

### CIP Services

#### Common Services (0x01 - 0x1C)

| Service Code | Name | Description |
|--------------|------|-------------|
| 0x01 | Get_Attribute_All | Retrieve all attributes of an object instance |
| 0x02 | Set_Attribute_All | Set all attributes of an object instance |
| 0x03 | Get_Attribute_List | Retrieve specific attributes by list |
| 0x04 | Set_Attribute_List | Set specific attributes by list |
| 0x05 | Reset | Reset an object instance |
| 0x06 | Start | Start an object instance |
| 0x07 | Stop | Stop an object instance |
| 0x08 | Create | Create a new object instance |
| 0x09 | Delete | Delete an object instance |
| 0x0A | Multiple_Service_Packet | Execute multiple services in one request |
| 0x0D | Apply_Attributes | Apply pending attribute changes |
| 0x0E | Get_Attribute_Single | Retrieve a single attribute |
| 0x10 | Set_Attribute_Single | Set a single attribute |
| 0x11 | Find_Next | Find next object instance matching criteria |
| 0x15 | Restore | Restore object from backup |
| 0x16 | Save | Save object to backup |
| 0x17 | No_Operation | No operation (keep-alive) |
| 0x1C | Group_Sync | Synchronize multiple objects |

#### Object-Specific Services

| Service Code | Name | Object | Description |
|--------------|------|--------|-------------|
| 0x4C | Get_And_Clear | Ethernet Link | Get and clear interface counters |
| 0x4E | Forward_Close | Connection Manager | Close a connection |
| 0x52 | Unconnected_Send | Message Router | Send unconnected message |
| 0x54 | Forward_Open | Connection Manager | Open a connection |
| 0x56 | Get_Connection_Data | Connection Manager | Get connection information |
| 0x57 | Search_Connection_Data | Connection Manager | Search for connection |
| 0x5A | Get_Connection_Owner | Connection Manager | Get connection owner |
| 0x5B | Large_Forward_Open | Connection Manager | Open large connection |

---

## CIP Object Reference

### Identity Object (Class Code: 0x01)

**Purpose**: Provides device identification and status information.

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes (Instance 1)

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Vendor ID | UINT | R | ODVA-assigned vendor identifier |
| 2 | Device Type | UINT | R | Device type code (e.g., 7 = Programmable Logic Controller) |
| 3 | Product Code | UINT | R | Vendor-specific product code |
| 4 | Revision | STRUCT | R | Major.Minor revision (USINT, USINT) |
| 5 | Status | WORD | R | Device status flags |
| 6 | Serial Number | UDINT | R | Unique device serial number |
| 7 | Product Name | STRING | R | Human-readable product name |

#### Status Word (Attribute 5) Bits

- **Bit 0**: Owned - Device is owned by a scanner
- **Bit 1**: Configured - Device is configured
- **Bit 2**: Reserved
- **Bit 3**: Reserved
- **Bit 4**: Reserved
- **Bit 5**: Reserved
- **Bit 6**: Minor Recoverable Fault
- **Bit 7**: Minor Unrecoverable Fault
- **Bit 8**: Major Recoverable Fault
- **Bit 9**: Major Unrecoverable Fault
- **Bit 10**: Reserved
- **Bit 11**: Reserved
- **Bit 12**: Reserved
- **Bit 13**: Reserved
- **Bit 14**: Reserved
- **Bit 15**: Extended Device Status

#### Supported Services

- Get_Attribute_All (0x01)
- Get_Attribute_Single (0x0E)
- Set_Attribute_Single (0x10) - Limited attributes
- Reset (0x05) - Type 0 and Type 1 reset

### Message Router Object (Class Code: 0x02)

**Purpose**: Routes CIP messages to appropriate objects.

#### Supported Services

- Unconnected_Send (0x52) - Route unconnected messages
- Connected messages are automatically routed

### Assembly Object (Class Code: 0x04)

**Purpose**: Groups data for I/O connections (implicit messaging).

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Data | ARRAY of BYTE | R/W | Assembly data content |
| 2 | Data Size | UINT | R | Size of data array in bytes |
| 3 | Connection Point List | ARRAY | R | List of connection points using this assembly |

#### Common Assembly Instances

- **100 (0x64)**: Input Assembly - Data produced by device (sent to scanner)
- **150 (0x96)**: Output Assembly - Data consumed by device (received from scanner)
- **151 (0x97)**: Configuration Assembly - Configuration data

#### Supported Services

- Get_Attribute_All (0x01)
- Get_Attribute_Single (0x0E)
- Set_Attribute_Single (0x10)

### Connection Manager Object (Class Code: 0x06)

**Purpose**: Manages explicit and implicit connections.

#### Class Attributes

- **Revision**: Object class revision number

#### Supported Services

- Forward_Open (0x54) - Establish connection
- Large_Forward_Open (0x5B) - Establish large connection
- Forward_Close (0x4E) - Close connection
- Get_Connection_Data (0x56) - Get connection information
- Search_Connection_Data (0x57) - Search connections
- Get_Connection_Owner (0x5A) - Get connection owner

### TCP/IP Interface Object (Class Code: 0xF5)

**Purpose**: Provides TCP/IP network configuration and status.

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes (Instance 1)

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Status | DWORD | R | Interface status flags |
| 2 | Configuration Capability | DWORD | R | Supported configuration methods |
| 3 | Configuration Control | DWORD | R/W | Active configuration method |
| 4 | Physical Link Object | UINT | R | Link to Ethernet Link object instance |
| 5 | Interface Configuration | STRUCT | R/W | IP configuration (IP, subnet, gateway) |
| 6 | Host Name | STRING | R/W | Device hostname |
| 7 | Domain Name | STRING | R/W | Domain name |
| 8 | Name Server | ARRAY | R/W | DNS server addresses |
| 9 | Name Server 2 | ARRAY | R/W | Secondary DNS server addresses |
| 10 | Select ACD | BOOL | R/W | Enable Address Conflict Detection |
| 11 | ACD Timeout | UINT | R/W | ACD timeout in seconds |
| 12 | ACD Count | UINT | R | Number of ACD conflicts detected |
| 13 | Encapsulation Inactivity Timeout | UINT | R/W | Timeout for encapsulation inactivity |

#### Configuration Control Values (Attribute 3)

- **0**: Static IP configuration
- **1**: BOOTP configuration
- **2**: DHCP configuration
- **3**: Reserved
- **4**: Reserved

#### Supported Services

- Get_Attribute_All (0x01)
- Get_Attribute_Single (0x0E)
- Set_Attribute_Single (0x10)

### Ethernet Link Object (Class Code: 0xF6)

**Purpose**: Provides Ethernet physical layer information and statistics.

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes (Instance 1)

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Interface Speed | UINT | R | Link speed (Mbps) |
| 2 | Interface Flags | DWORD | R | Interface capabilities and status |
| 3 | Physical Address | ARRAY[6] | R | MAC address |
| 4 | Interface Counters | STRUCT | R | Receive/transmit statistics |
| 5 | Media Counters | STRUCT | R | Physical layer statistics |
| 6 | Interface Type | UINT | R | Interface type (e.g., Ethernet) |
| 7 | Interface State | UINT | R | Current interface state |
| 8 | Admin State | UINT | R/W | Administrative state control |

#### Interface Speed Values (Attribute 1)

- **10**: 10 Mbps
- **100**: 100 Mbps
- **1000**: 1000 Mbps (Gigabit)

#### Interface State Values (Attribute 7)

- **0**: Unknown
- **1**: Operational
- **2**: Faulted
- **3**: Not Configured

#### Supported Services

- Get_Attribute_All (0x01)
- Get_Attribute_Single (0x0E)
- Set_Attribute_Single (0x10) - Limited attributes
- Get_And_Clear (0x4C) - Clear counters

### Quality of Service Object (Class Code: 0x48)

**Purpose**: Configures Quality of Service (QoS) parameters for network traffic.

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes (Instance 1)

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | 802.1Q Enable | BOOL | R/W | Enable 802.1Q VLAN tagging |
| 2 | PTP DSCP Override | UINT | R/W | DSCP value for PTP traffic |
| 3 | PTP DSCP Override Enable | BOOL | R/W | Enable PTP DSCP override |
| 4 | Urgent DSCP | UINT | R/W | DSCP for urgent traffic (default: 55) |
| 5 | Scheduled DSCP | UINT | R/W | DSCP for scheduled traffic (default: 47) |
| 6 | High DSCP | UINT | R/W | DSCP for high priority (default: 43) |
| 7 | Low DSCP | UINT | R/W | DSCP for low priority (default: 31) |
| 8 | Explicit DSCP | UINT | R/W | DSCP for explicit messages (default: 27) |

#### DSCP Priority Levels

- **Urgent (55)**: Critical real-time I/O
- **Scheduled (47)**: Scheduled I/O updates
- **High (43)**: High priority data
- **Low (31)**: Normal priority data
- **Explicit (27)**: Configuration and diagnostics

#### Supported Services

- Get_Attribute_All (0x01)
- Get_Attribute_Single (0x0E)
- Set_Attribute_Single (0x10)

### Device Level Ring Object (Class Code: 0x47)

**Purpose**: Manages Device Level Ring (DLR) topology for redundant ring networks.

**Note**: Not applicable to ESP32-P4 due to single Ethernet port limitation.

#### Class Attributes

- **Revision**: Object class revision number

#### Instance Attributes (Instance 1)

| Attribute ID | Name | Type | Access | Description |
|--------------|------|------|--------|-------------|
| 1 | Network Topology | UINT | R | Current network topology |
| 2 | Network Status | UINT | R | Ring status |
| 10 | Active Supervisor Address | UINT | R | Ring supervisor MAC address |
| 12 | Capability Flags | DWORD | R | Device DLR capabilities |

#### Network Topology Values (Attribute 1)

- **0**: Unknown
- **1**: Linear
- **2**: Ring

#### Network Status Values (Attribute 2)

- **0**: Unknown
- **1**: Healthy Ring
- **2**: Faulted Ring
- **3**: Ring Not Present

#### Supported Services

- Get_Attribute_All (0x01)
- Get_Attribute_Single (0x0E)

---

## Connection Management

### Connection Types

EtherNet/IP supports two main connection types:

1. **Explicit Connections (Class 3)**
   - Request-response messaging
   - Used for configuration and diagnostics
   - TCP-based (port 44818 / 0xAF12)
   - Connection-oriented

2. **Implicit Connections (Class 1)**
   - Real-time I/O data exchange
   - Cyclic data transfer
   - UDP-based (port 2222 / 0x08AE)
   - Connection-oriented with timing requirements

### Connection Transport Classes

| Class | Name | Description |
|-------|------|-------------|
| 0 | Null | No transport |
| 1 | Server | Server-side connection |
| 2 | Client | Client-side connection |
| 3 | Transport Class 3 | Reserved |
| 4 | Transport Class 4 | Reserved |
| 5 | Transport Class 5 | Reserved |
| 6 | Transport Class 6 | Reserved |
| 7 | Transport Class 7 | Reserved |

### Connection Application Types

| Type | Name | Description |
|------|------|-------------|
| 0 | Null | No application |
| 1 | Explicit | Explicit messaging |
| 2 | I/O | Implicit I/O messaging |
| 3 | COS | Change of State |
| 4 | Cyclic | Cyclic I/O updates |

### Forward_Open Request

Establishes a new connection. Key parameters:

- **Connection Path**: Path to target object
- **O→T Connection ID**: Originator to Target connection identifier
- **T→O Connection ID**: Target to Originator connection identifier
- **Connection Serial Number**: Unique connection identifier
- **Vendor ID**: Vendor identifier
- **Originator Serial Number**: Originator device serial number
- **Connection Timeout Multiplier**: Timeout calculation factor
- **O→T RPI**: Requested Packet Interval (Originator to Target)
- **T→O RPI**: Requested Packet Interval (Target to Originator)
- **Transport Class**: Connection transport class
- **Connection Type**: Connection application type
- **Connection Priority**: Connection priority level
- **Connection Size**: Data size for connection

### Forward_Close Request

Closes an established connection. Key parameters:

- **Connection Path**: Path to connection object
- **Connection Serial Number**: Connection identifier to close
- **Vendor ID**: Vendor identifier
- **Originator Serial Number**: Originator device serial number

### Connection States

- **Non-existent**: Connection does not exist
- **Configuring**: Connection being established
- **Established**: Connection active and transferring data
- **Timed Out**: Connection timed out
- **Deferred Delete**: Connection marked for deletion
- **Closing**: Connection being closed

### Connection Timeout

Connection timeout is calculated as:
```
Timeout = RPI × Connection Timeout Multiplier
```

Typical timeout multipliers:
- **0**: 4× RPI
- **1**: 8× RPI
- **2**: 16× RPI
- **3**: 32× RPI

---

## Messaging Types

### Explicit Messaging

**Purpose**: Configuration, diagnostics, and non-real-time data exchange.

**Characteristics**:
- Request-response model
- TCP-based (reliable)
- Port 44818 (0xAF12)
- Connection-oriented
- Variable data size

**Message Flow**:
1. Scanner sends explicit request
2. Adapter processes request
3. Adapter sends response
4. Connection may remain open for multiple requests

**Common Uses**:
- Reading/writing CIP object attributes
- Device configuration
- Connection establishment (Forward_Open)
- Connection closure (Forward_Close)
- Device discovery (List Identity)

### Implicit Messaging

**Purpose**: Real-time cyclic I/O data exchange.

**Characteristics**:
- Producer-consumer model
- UDP-based (low latency)
- Port 2222 (0x08AE)
- Connection-oriented
- Fixed data size per connection
- Cyclic updates at RPI interval

**Message Flow**:
1. Connection established via Forward_Open
2. Scanner sends output data cyclically (O→T)
3. Adapter sends input data cyclically (T→O)
4. Data exchanged at RPI rate
5. Connection timeout if no data received

**Connection Points**:
- **Exclusive Owner**: Scanner has exclusive control
- **Input Only**: Scanner only receives data (read-only)
- **Listen Only**: Scanner receives data but doesn't control outputs

### Run/Idle Header

Optional 4-byte header in implicit messages:

```
Byte 0-1: Sequence Count (UINT)
Byte 2:   Run/Idle Status (BOOL)
Byte 3:   Reserved
```

- **Run Status**: Device is in run mode (1) or idle mode (0)
- **Sequence Count**: Increments with each data change

---

## Error Codes and Status

### General CIP Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | Success | Operation successful |
| 0x01 | Connection Failure | Connection failed |
| 0x02 | Resource Unavailable | Resource not available |
| 0x03 | Invalid Parameter Value | Parameter value invalid |
| 0x04 | Path Segment Error | Invalid path segment |
| 0x05 | Path Destination Unknown | Path destination not found |
| 0x06 | Partial Transfer | Partial data transferred |
| 0x07 | Connection Lost | Connection lost |
| 0x08 | Service Not Supported | Service not supported |
| 0x09 | Invalid Attribute Value | Attribute value invalid |
| 0x0A | Attribute List Error | Attribute list error |
| 0x0B | Already In Requested Mode | Already in requested state |
| 0x0C | Object State Conflict | Object state conflict |
| 0x0D | Object Already Exists | Object already exists |
| 0x0E | Attribute Not Settable | Attribute is read-only |
| 0x0F | Privilege Violation | Insufficient privileges |
| 0x10 | Device State Conflict | Device state conflict |
| 0x11 | Reply Data Too Large | Response too large |
| 0x12 | Fragmentation Of Primitive Value | Fragmentation error |
| 0x13 | Not Enough Data | Insufficient data |
| 0x14 | Attribute Not Supported | Attribute not supported |
| 0x15 | Too Much Data | Too much data provided |
| 0x16 | Object Does Not Exist | Object not found |
| 0x17 | Service Fragmentation Sequence Not In Progress | Fragmentation error |
| 0x18 | No Stored Attribute Data | No stored data |
| 0x19 | Store Operation Failure | Store operation failed |
| 0x1A | Routing Failure Request Packet Too Large | Routing failure |
| 0x1B | Routing Failure Response Packet Too Large | Routing failure |
| 0x1C | Missing Attribute List Entry Data | Missing attribute data |
| 0x1D | Invalid Attribute Value List | Invalid attribute list |
| 0x1E | Embedded Service Error | Embedded service error |
| 0x1F | Vendor Specific Error | Vendor-specific error |
| 0x20 | Invalid Parameter | Invalid parameter |
| 0x21 | Write Once Value Or Medium Already Written | Write-once value already set |
| 0x22 | Invalid Reply Received | Invalid reply |
| 0x23 | Buffer Overflow | Buffer overflow |
| 0x24 | Message Format Error | Message format error |
| 0x25 | Key Failure In Path | Key failure |
| 0x26 | Path Size Invalid | Invalid path size |
| 0x27 | Unexpected Attribute In List | Unexpected attribute |
| 0x28 | Invalid Member ID | Invalid member ID |
| 0x29 | Member Not Settable | Member not settable |
| 0x2A | Group 2 Only Server General Failure | Group 2 server failure |
| 0x2B | Unknown Modbus Error | Modbus error |

### Connection Manager Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x0100 | Connection In Use | Connection already in use |
| 0x0101 | Transport Not Supported | Transport class not supported |
| 0x0102 | Owner Conflict | Connection owner conflict |
| 0x0103 | Connection Not Found | Connection not found |
| 0x0104 | Invalid Connection Type | Invalid connection type |
| 0x0105 | Invalid Connection Size | Invalid connection size |
| 0x0106 | Module Not Configured | Module not configured |
| 0x0107 | EKey | Invalid connection key |
| 0x0108 | Connection Rejected | Connection rejected |
| 0x0109 | Connection Path Not Available | Connection path unavailable |
| 0x010A | Invalid Connection Path | Invalid connection path |
| 0x010B | Resource Unavailable | Resource unavailable |
| 0x010C | Invalid Originator To Target Connection ID | Invalid O→T connection ID |
| 0x010D | Invalid Target To Originator Connection ID | Invalid T→O connection ID |
| 0x010E | Invalid Network Segment | Invalid network segment |
| 0x010F | Connection Not Established | Connection not established |

### Status Codes

#### Module Status Values

- **0**: Non-existent
- **1**: Configuring
- **2**: Running
- **3**: Running Degraded
- **4**: Stopped
- **5**: Faulted
- **6**: Faulted Degraded

#### Network Status Values

- **0**: Unknown
- **1**: Normal
- **2**: Degraded
- **3**: Faulted

---

## Protocol Details

### EtherNet/IP Encapsulation

EtherNet/IP uses an encapsulation protocol over TCP/UDP.

#### Encapsulation Header Structure

```
Offset  Length  Field
0       2       Command
2       2       Length
4       4       Session Handle
8       4       Status
12      8       Sender Context
20      4       Options
```

#### Common Encapsulation Commands

| Code | Name | Description |
|------|------|-------------|
| 0x0063 | RegisterSession | Register encapsulation session |
| 0x0064 | UnregisterSession | Unregister session |
| 0x0065 | SendRRData | Send request-response data |
| 0x0066 | SendUnitData | Send unit data |
| 0x0067 | IndicateStatus | Status indication |
| 0x0068 | Cancel | Cancel request |
| 0x006F | ListServices | List available services |
| 0x0070 | ListIdentity | List device identity |
| 0x0071 | ListInterfaces | List network interfaces |

### Port Numbers

- **TCP Port 44818 (0xAF12)**: Explicit messaging
- **UDP Port 44818 (0xAF12)**: Explicit messaging (unconnected)
- **UDP Port 2222 (0x08AE)**: Implicit I/O messaging

### Connection Path Format

Connection paths use EPATH format:

```
Format: [Port, Link, ...] Class, Instance, Attribute
```

**Example Paths**:
- `20 04 24 01` = Class 0x01, Instance 1 (Identity Object)
- `20 04 24 02` = Class 0x02, Instance 1 (Message Router)
- `20 04 24 06` = Class 0x06, Instance 1 (Connection Manager)

### RPI (Requested Packet Interval)

- **Purpose**: Defines cyclic update rate for implicit connections
- **Units**: Microseconds (μs)
- **Range**: Typically 1ms to 1s
- **Common Values**:
  - 10ms (10,000 μs) - Fast I/O
  - 100ms (100,000 μs) - Standard I/O
  - 1s (1,000,000 μs) - Slow updates

### Watchdog Handling

- Connections have timeout mechanisms
- Missing data triggers timeout
- Timeout = RPI × Timeout Multiplier
- Device should handle timeout gracefully
- May transition to fault state on timeout

### Data Change Detection

- Sequence counters track data changes
- Incremented when data changes
- Helps scanners detect updates
- Reduces unnecessary processing

---

## Implementation Notes

### Adapter Responsibilities

1. **Respond to Explicit Messages**
   - Process CIP service requests
   - Return appropriate responses
   - Handle error conditions

2. **Manage Connections**
   - Accept Forward_Open requests
   - Maintain connection state
   - Handle Forward_Close requests
   - Monitor connection timeouts

3. **Exchange I/O Data**
   - Produce input data at RPI rate
   - Consume output data from scanner
   - Update sequence counters
   - Handle Run/Idle status

4. **Maintain Object State**
   - Keep object attributes current
   - Update status flags
   - Handle state transitions

### Best Practices

1. **Error Handling**
   - Return appropriate error codes
   - Log errors for debugging
   - Handle timeout conditions gracefully

2. **Performance**
   - Meet RPI requirements
   - Minimize processing latency
   - Optimize data transfer

3. **Reliability**
   - Validate all inputs
   - Handle connection failures
   - Maintain state consistency

4. **Compliance**
   - Follow CIP specification
   - Support required objects
   - Implement required services

---

## References

- **OpENer Stack**: Open-source EtherNet/IP implementation
- **Hilscher EtherNet/IP Scanner Protocol API V2.10.0**: Source document for this reference

---

*Project: ESP32-P4 EtherNet/IP Adapter Implementation*

