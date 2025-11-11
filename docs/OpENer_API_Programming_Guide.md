# OpENer CIP API Programming Guide

This document provides a comprehensive guide to using the OpENer CIP API for implementing EtherNet/IP enabled devices. OpENer is an open-source EtherNet/IP stack that provides a portable implementation of the Common Industrial Protocol (CIP) over Ethernet.

**Source**: [OpENer CIP API Documentation](https://eipstackgroup.github.io/OpENer/d2/dc9/group__CIP__API.html)

## Table of Contents

1. [Overview](#overview)
2. [Architecture and Design Philosophy](#architecture-and-design-philosophy)
3. [Core API Functions](#core-api-functions)
4. [Callback Functions](#callback-functions)
5. [CIP Object Management](#cip-object-management)
6. [Connection Management](#connection-management)
7. [Best Practices](#best-practices)
8. [Common Patterns](#common-patterns)

## Overview

### What is OpENer?

OpENer is an open-source EtherNet/IP I/O Target (slave device) stack written in C. It provides a complete implementation of the CIP protocol layer, allowing embedded devices to communicate as EtherNet/IP devices. The stack handles:

- **Explicit Messaging**: Request-response communication for configuration and diagnostics
- **Implicit I/O**: Real-time cyclic data exchange for control applications
- **Connection Management**: Establishing and maintaining CIP connections
- **CIP Objects**: Standard objects (Identity, Message Router, Connection Manager, etc.)

### Why Use OpENer?

1. **Portability**: Designed for embedded systems with minimal dependencies
2. **Standards Compliance**: Implements ODVA EtherNet/IP specification
3. **Open Source**: Free to use, modify, and distribute
4. **Mature**: Actively maintained and used in production systems
5. **Modular**: Clean API allows integration with existing applications

## Architecture and Design Philosophy

### Object-Oriented Design in C

OpENer uses an object-oriented design pattern implemented in C. This approach provides:

- **Encapsulation**: Each CIP object manages its own state and behavior
- **Modularity**: Objects can be added, removed, or modified independently
- **Extensibility**: New CIP objects can be created without modifying core stack

### Callback-Based Architecture

OpENer uses callbacks extensively to notify the application layer of events. This design:

- **Decouples Stack from Application**: Stack doesn't need to know application details
- **Enables Flexibility**: Application can respond to events as needed
- **Maintains Performance**: No polling required; events trigger callbacks directly

### Why This Architecture?

1. **Resource Efficiency**: Callbacks avoid polling overhead in embedded systems
2. **Real-Time Performance**: Event-driven design supports deterministic behavior
3. **Separation of Concerns**: Stack handles protocol, application handles business logic
4. **Testability**: Callbacks can be mocked for unit testing

## Core API Functions

### Stack Initialization and Shutdown

#### `EipStatus ConfigureExplicitMessaging(EipUint16 explicit_message_connection_size)`

**Purpose**: Configure the maximum number of explicit message connections.

**How it works**: Allocates memory for explicit message connection objects. Explicit messages are used for configuration, diagnostics, and non-real-time communication.

**Why it's needed**: Limits memory usage by pre-allocating connection objects. Prevents memory fragmentation in embedded systems.

**Parameters**:
- `explicit_message_connection_size`: Maximum number of simultaneous explicit connections

**Returns**: `kEipStatusOk` on success, error code on failure

**Example**:
```c
EipStatus status = ConfigureExplicitMessaging(4);  // Allow up to 4 explicit connections
if (status != kEipStatusOk) {
    // Handle error
}
```

#### `EipStatus ShutdownCipStack(void)`

**Purpose**: Gracefully shutdown the CIP stack and free all resources.

**How it works**: 
1. Closes all open I/O connections
2. Closes all open explicit connections
3. Frees all memory allocated by the stack

**Why it's important**: 
- Prevents resource leaks
- Ensures clean shutdown
- Allows stack re-initialization if needed

**Note**: Memory allocated by the application is NOT freed. Application must manage its own memory.

**Example**:
```c
void application_shutdown(void) {
    // Clean up application resources first
    cleanup_application_data();
    
    // Then shutdown CIP stack
    ShutdownCipStack();
}
```

### Device Identity Management

#### `void SetDeviceSerialNumber(const EipUint32 serial_number)`

**Purpose**: Set the unique serial number for the device's Identity object.

**How it works**: Updates the Identity object's serial number attribute (attribute 4). This value must be unique per manufacturer.

**Why it's needed**: 
- Identifies specific device instances
- Required for device discovery and identification
- Used in device replacement scenarios

**Parameters**:
- `serial_number`: Unique 32-bit serial number (unique per manufacturer)

**Example**:
```c
// Set serial number during device initialization
SetDeviceSerialNumber(0x12345678);  // Unique device serial number
```

**Best Practice**: Use a value that's truly unique. Consider combining manufacturer ID with production date/time or MAC address.

#### `void SetDeviceStatus(const EipUint16 status)`

**Purpose**: Update the device status in the Identity object.

**How it works**: Updates the Identity object's status attribute (attribute 1). Status indicates device operational state.

**Why it's needed**:
- Communicates device state to scanners/controllers
- Enables diagnostics and troubleshooting
- Required for proper device operation

**Status Values** (from CIP specification):
- `0x0000`: Owned (device is owned by a connection)
- `0x0001`: Configured (device is configured)
- `0x0002`: In use (device is actively being used)
- `0x0003`: Faulted (device has a fault)

**Example**:
```c
// Set device status during initialization
SetDeviceStatus(0x0001);  // Device is configured and ready

// Later, when fault occurs
SetDeviceStatus(0x0003);  // Device is faulted

// After fault clears
SetDeviceStatus(0x0001);  // Device is configured again
```

## CIP Object Management

### Creating CIP Objects

#### `EipStatus InsertAttribute(CipInstance *const cip_instance, EipUint16 attribute_number, EipUint8 cip_data_type, void *const cip_data, EipByte cip_flags)`

**Purpose**: Add an attribute to a CIP class instance.

**How it works**: 
1. Validates parameters
2. Finds or creates attribute entry in instance's attribute array
3. Sets attribute data, type, and access flags
4. If attribute already exists, replaces it

**Why this design**:
- **Pre-allocation**: Attributes are stored in fixed-size arrays (not expandable)
- **Efficiency**: Direct array access for fast attribute reads/writes
- **Simplicity**: Single function handles all attribute types

**Parameters**:
- `cip_instance`: Pointer to CIP class instance (typically Instance 0)
- `attribute_number`: Attribute number (as defined in CIP specification)
- `cip_data_type`: CIP data type (e.g., `kCipUsint`, `kCipUdint`, `kCipString`)
- `cip_data`: Pointer to attribute data
- `cip_flags`: Access flags (`kGetableSingle`, `kGetableAll`, `kSetable`)

**Access Flags**:
- `kGetableSingle`: Attribute can be read individually
- `kGetableAll`: Attribute included in Get_Attribute_All response
- `kSetable`: Attribute can be written (configuration)

**Example**:
```c
// Add Vendor ID attribute to Identity object (Instance 0)
EipUint16 vendor_id = 55512;  // Your vendor ID
InsertAttribute(
    GetIdentityInstance(),           // Instance 0 of Identity class
    2,                               // Attribute 2: Vendor ID
    kCipUdint,                       // Data type: UDINT (32-bit unsigned)
    &vendor_id,                      // Pointer to data
    kGetableSingle | kGetableAll     // Readable but not writable
);

// Add Product Name attribute
const char *product_name = "ESP32P4-EIP";
InsertAttribute(
    GetIdentityInstance(),
    7,                               // Attribute 7: Product Name
    kCipShortString,                 // Data type: Short String
    (void*)product_name,
    kGetableSingle | kGetableAll
);
```

**Important Notes**:
- Attributes array is **not expandable** - must be sized correctly at initialization
- Inserting an existing attribute **replaces** the previous one
- Attribute data pointer must remain valid for object lifetime
- Use appropriate CIP data types as defined in specification

#### `void InsertService(const CipClass *const cip_class_to_add_service, EipUint8 service_code, const CipServiceFunction service_function, char *const service_name)`

**Purpose**: Register a CIP service handler for a CIP class.

**How it works**:
1. Validates service code and function pointer
2. Adds service to class's service array
3. Associates service code with handler function
4. If service already exists, replaces it

**Why this design**:
- **Extensibility**: Allows custom services for vendor-specific objects
- **Flexibility**: Services can be added/modified at runtime
- **Standards Compliance**: Supports both standard and custom services

**Parameters**:
- `cip_class_to_add_service`: Pointer to CIP class (may be Instance 0)
- `service_code`: CIP service code (e.g., `0x01` = Get, `0x05` = Reset)
- `service_function`: Function pointer implementing the service
- `service_name`: Human-readable service name (for debugging)

**Service Function Signature**:
```c
typedef EipStatus (*CipServiceFunction)(
    CipInstance *const instance,
    CipMessageRouterRequest *const message_router_request,
    CipMessageRouterResponse *const message_router_response,
    struct sockaddr *originator_address,
    const int encapsulation_session
);
```

**Example**:
```c
// Custom service handler
EipStatus MyCustomService(
    CipInstance *const instance,
    CipMessageRouterRequest *const request,
    CipMessageRouterResponse *const response,
    struct sockaddr *originator_address,
    const int encapsulation_session
) {
    // Process service request
    // Set response data
    response->general_status = kCipGeneralStatusCodeSuccess;
    return kEipStatusOk;
}

// Register custom service
InsertService(
    GetMyCustomClass(),      // Your CIP class
    0x99,                    // Custom service code (0x80-0xFF are vendor-specific)
    MyCustomService,         // Handler function
    "MyCustomService"        // Service name
);
```

**Important Notes**:
- Service array is **not expandable** - must be sized correctly
- Service codes `0x80-0xFF` are vendor-specific (custom services)
- Service function must handle all error cases
- Service name is for debugging/logging only

## Connection Management

### Connection Lifecycle

CIP connections go through several states:
1. **Non-existent**: Connection doesn't exist
2. **Configuring**: Connection is being established
3. **Established**: Connection is active and transferring data
4. **Timed-out**: Connection watchdog expired
5. **Closed**: Connection terminated

### Managing Connections

#### `EipStatus ManageConnections(MilliSeconds elapsed_time)`

**Purpose**: Check and manage connection timers (transmission triggers and watchdogs).

**How it works**:
1. Iterates through all active connections
2. Checks transmission trigger timers (RPI-based)
3. Checks watchdog timers (inactivity timeout)
4. Triggers data production for cyclic connections
5. Handles timeouts and connection state changes

**Why periodic calls are needed**:
- **Timer Management**: Stack doesn't have its own timer thread
- **Resource Efficiency**: Application controls when stack processing occurs
- **Deterministic Behavior**: Predictable timing for real-time applications

**Parameters**:
- `elapsed_time`: Milliseconds since last call (allows handling of missed calls)

**Returns**: `kEipStatusOk` on success

**Example**:
```c
void application_main_loop(void) {
    MilliSeconds last_time = GetSystemTimeMs();
    
    while (running) {
        MilliSeconds current_time = GetSystemTimeMs();
        MilliSeconds elapsed = current_time - last_time;
        
        // Call every tick (typically 1-10ms)
        ManageConnections(elapsed);
        
        // Handle application logic
        HandleApplication();
        
        // Process network messages
        NetworkHandlerProcessOnce();
        
        last_time = current_time;
        
        // Small delay to prevent CPU spinning
        DelayMs(1);
    }
}
```

**Critical Timing**:
- Call frequency: Typically every 1-10ms (`kOpenerTimerTickInMilliSeconds`)
- If more time elapsed, pass actual elapsed time (simplifies algorithm)
- Must be called regularly for connections to function properly

**What Happens Inside**:
1. **Transmission Trigger**: For cyclic connections, checks if RPI interval elapsed
2. **Production Inhibit**: Respects production inhibit timer (prevents excessive transmission)
3. **Watchdog**: Checks if connection received data within timeout period
4. **Timeout Handling**: Calls `ConnectionTimeoutFunction` if watchdog expires
5. **Data Production**: Calls `ConnectionSendDataFunction` for cyclic connections

#### `EipStatus TriggerConnections(unsigned int output_assembly_id, unsigned int input_assembly_id)`

**Purpose**: Manually trigger production of an application-triggered connection.

**How it works**:
1. Finds connection matching assembly IDs
2. Verifies connection is established and application-triggered
3. Schedules production at next opportunity (respects RPI and inhibit timer)
4. Application notified via `BeforeAssemblyDataSend` callback

**Why application-triggered connections**:
- **Event-Driven**: Data sent when application event occurs (not time-based)
- **Efficiency**: Reduces unnecessary network traffic
- **Flexibility**: Application controls when data is sent

**Parameters**:
- `output_assembly_id`: Output assembly connection point
- `input_assembly_id`: Input assembly connection point

**Returns**: `kEipStatusOk` on success, error if connection not found or not triggerable

**Example**:
```c
// Application detects significant change in sensor value
void sensor_value_changed(void) {
    // Trigger connection to send updated data
    TriggerConnections(100, 150);  // Output assembly 100, Input assembly 150
}

// Or trigger when button pressed
void button_pressed(void) {
    TriggerConnections(100, 150);
}
```

**Important Notes**:
- Only works for application-triggered connections (not cyclic)
- Connection must be in Established state
- Production respects RPI and production inhibit timer
- Should only be called from `HandleApplication()` context

## Callback Functions

**CRITICAL IMPORTANCE**: Callbacks are the **primary interface** between OpENer and your application. They are called by the stack at critical points to:
- Notify your application of events
- Request data from your application
- Allow your application to process received data
- Enable your application to control connection behavior

**Why Callbacks Are Essential**:
1. **Event-Driven Architecture**: OpENer uses callbacks to notify your application immediately when events occur
2. **Real-Time Performance**: No polling overhead - events trigger callbacks directly
3. **Separation of Concerns**: Stack handles protocol, callbacks handle application logic
4. **Deterministic Behavior**: Callbacks are called at predictable times, enabling real-time control

**Callback Execution Context**:
- Most callbacks are called from the OpENer task context (typically Core 0)
- Callbacks should execute quickly - avoid blocking operations
- Use queues or flags to defer heavy processing to application task
- Be aware of thread safety if accessing shared resources

OpENer requires the application to implement several callback functions. These allow the stack to interact with the application layer.

### Connection Callbacks

#### `typedef CipError (*OpenConnectionFunction)(CipConnectionObject *RESTRICT const connection_object, EipUint16 *const extended_error_code)`

**Purpose**: Called when a new connection is being established.

**How it works**: Stack calls this function before connection becomes active. Application can:
- Initialize connection-specific resources
- Validate connection parameters
- Prepare data buffers
- Set extended error code if connection should be rejected

**Why it's needed**: Allows application to prepare for connection before data transfer begins.

**Parameters**:
- `connection_object`: Connection object being opened
- `extended_error_code`: Pointer to extended error code (set if returning error)

**Returns**: 
- `kCipErrorSuccess` if connection can be opened
- Other `CipError` values to reject connection (e.g., `kCipErrorConnectionFailure`)

**Example**:
```c
CipError MyOpenConnection(CipConnectionObject *connection_object,
                          EipUint16 *extended_error_code) {
    // Allocate connection-specific resources
    MyConnectionData *conn_data = malloc(sizeof(MyConnectionData));
    if (!conn_data) {
        *extended_error_code = 0;  // No extended error code
        return kCipErrorResourceUnavailable;
    }
    
    // Store in connection object (if you extend it)
    // connection_object->application_data = conn_data;
    
    // Initialize connection data
    conn_data->buffer = malloc(connection_object->produced_connection_size);
    if (!conn_data->buffer) {
        free(conn_data);
        *extended_error_code = 0;
        return kCipErrorResourceUnavailable;
    }
    
    return kCipErrorSuccess;
}
```

**Note**: This callback is typically used for advanced connection management. Most applications don't need to implement it unless they require connection-specific resource management.

#### `typedef EipStatus (*ConnectionSendDataFunction)(CipConnectionObject *connection_object)`

**Purpose**: Called when connection should send data (for producing connections).

**How it works**: 
- For cyclic connections: Called at RPI intervals
- For application-triggered: Called when `TriggerConnections()` invoked
- Application fills output assembly data
- Stack handles actual network transmission

**Why callback-based**:
- **Separation of Concerns**: Stack handles protocol, application handles data
- **Flexibility**: Application controls data preparation timing
- **Performance**: Data prepared just before sending (fresh data)

**Parameters**:
- `connection_object`: Connection that should send data

**Returns**: `kEipStatusOk` on success

**Example**:
```c
EipStatus MySendData(CipConnectionObject *connection_object) {
    // Get output assembly instance
    CipInstance *assembly = GetAssemblyInstance(connection_object->produced_path.instance_id);
    
    // Update assembly data with current sensor values
    EipUint8 *data = assembly->data;
    data[0] = read_sensor_1();
    data[1] = read_sensor_2();
    data[2] = read_sensor_3();
    
    return kEipStatusOk;
}
```

#### `typedef EipStatus (*ConnectionReceiveDataFunction)(CipConnectionObject *connection_object, const EipUint8 *data, const EipUint16 data_length)`

**Purpose**: Called when connection receives data (for consuming connections).

**How it works**: Stack receives data from network and calls this function. Application processes received data.

**Why callback-based**:
- **Immediate Processing**: Data processed as soon as received
- **Real-Time Response**: No polling delay
- **Event-Driven**: Application responds to data arrival

**Parameters**:
- `connection_object`: Connection that received data
- `data`: Pointer to received data
- `data_length`: Length of received data

**Returns**: `kEipStatusOk` on success

**Example**:
```c
EipStatus MyReceiveData(CipConnectionObject *connection_object, 
                        const EipUint8 *data, 
                        const EipUint16 data_length) {
    // Process received control data
    if (data_length >= 3) {
        set_actuator_1(data[0]);
        set_actuator_2(data[1]);
        set_actuator_3(data[2]);
    }
    
    return kEipStatusOk;
}
```

#### `typedef void (*ConnectionCloseFunction)(CipConnectionObject *connection_object)`

**Purpose**: Called when connection is being closed.

**How it works**: Stack calls this before closing connection. Application should clean up connection-specific resources.

**Why it's needed**: Prevents resource leaks when connections close.

**Parameters**:
- `connection_object`: Connection being closed

**Example**:
```c
void MyCloseConnection(CipConnectionObject *connection_object) {
    // Free connection-specific resources
    MyConnectionData *conn_data = connection_object->application_data;
    if (conn_data) {
        free(conn_data->buffer);
        free(conn_data);
        connection_object->application_data = NULL;
    }
}
```

#### `typedef void (*ConnectionTimeoutFunction)(CipConnectionObject *connection_object)`

**Purpose**: Called when connection watchdog timer expires.

**How it works**: Stack detects no data received within timeout period and calls this function.

**Why it's needed**: Allows application to handle connection failures gracefully.

**Parameters**:
- `connection_object`: Connection that timed out

**Example**:
```c
void MyConnectionTimeout(CipConnectionObject *connection_object) {
    // Handle timeout - perhaps enter safe state
    enter_safe_state();
    
    // Log timeout event
    log_error("Connection %d timed out", connection_object->connection_serial_number);
}
```

### Application Lifecycle Callbacks

#### `EipStatus ApplicationInitialization(void)`

**Purpose**: Called once after OpENer stack initialization is complete. This is where you create and configure your CIP objects.

**When it's called**: 
- Automatically called by `CipStackInit()` after stack initialization
- Called **once** during device startup
- Stack is ready to accept CIP objects at this point

**Why it exists**:
- **Convenience**: Provides a single place to initialize all application objects
- **Timing**: Ensures stack is fully initialized before creating objects
- **Organization**: Keeps initialization code together

**What to do here**:
- Create custom CIP objects
- Create Assembly objects for I/O
- Configure object attributes
- Register custom services
- Set up initial state

**What NOT to do**:
- Don't perform time-consuming operations (use `HandleApplication` instead)
- Don't assume network is ready (check link status separately)
- Don't create objects that depend on external hardware being ready

**Example**:
```c
EipStatus ApplicationInitialization(void) {
    EipStatus status = kEipStatusOk;
    
    // Create I/O assemblies
    // Input Assembly 100 (32 bytes)
    status = CreateAssemblyObject(100, 32, kEipBool8Array);
    if (status != kEipStatusOk) {
        OPENER_TRACE_ERR("Failed to create Input Assembly\n");
        return status;
    }
    
    // Output Assembly 150 (32 bytes)
    status = CreateAssemblyObject(150, 32, kEipBool8Array);
    if (status != kEipStatusOk) {
        OPENER_TRACE_ERR("Failed to create Output Assembly\n");
        return status;
    }
    
    // Configuration Assembly 151 (10 bytes)
    status = CreateAssemblyObject(151, 10, kEipBool8Array);
    if (status != kEipStatusOk) {
        OPENER_TRACE_ERR("Failed to create Config Assembly\n");
        return status;
    }
    
    // Create custom CIP object
    CreateMyCustomObject();
    
    // Initialize application-specific state
    initialize_application_state();
    
    OPENER_TRACE_INFO("Application initialization complete\n");
    return kEipStatusOk;
}
```

**ESP32-Specific Considerations**:
- This runs in the OpENer task context (Core 0)
- I2C/SPI hardware may not be initialized yet - defer hardware access
- Use this for object creation only, not hardware initialization

#### `void HandleApplication(void)`

**Purpose**: Called periodically during normal operation to allow application-specific processing.

**When it's called**:
- Called at the beginning of each `ManageConnections()` execution
- Called **very frequently** (typically every 1-10ms)
- Called from OpENer task context

**Why it exists**:
- **Periodic Processing**: Allows application to perform regular tasks
- **Integration Point**: Bridge between OpENer timing and application timing
- **State Updates**: Update application state, read sensors, control actuators

**What to do here**:
- Read sensor values (but don't update assemblies - use `BeforeAssemblyDataSend`)
- Update application state machines
- Check hardware status
- Process application logic
- **Keep execution time short** (< 1ms ideally)

**What NOT to do**:
- Don't perform blocking operations
- Don't update assembly data directly (use `BeforeAssemblyDataSend`)
- Don't make network calls
- Don't perform heavy computations (defer to separate task)

**Example**:
```c
void HandleApplication(void) {
    // Read sensors and update internal state
    sensor_values.temperature = read_temperature_sensor();
    sensor_values.pressure = read_pressure_sensor();
    sensor_values.status = read_status_bits();
    
    // Update application state machine
    update_state_machine();
    
    // Check for fault conditions
    if (check_fault_conditions()) {
        SetDeviceStatus(0x0003);  // Faulted
    }
    
    // Process application logic
    process_control_logic();
    
    // Note: Don't update assembly data here!
    // That happens in BeforeAssemblyDataSend callback
}
```

**Performance Critical**: This callback is called very frequently. Keep it fast!

### Assembly Data Callbacks

#### `EipStatus AfterAssemblyDataReceived(CipInstance *instance)`

**Purpose**: Called immediately after data is received for a consuming assembly (input assembly from scanner perspective).

**When it's called**:
- Called **synchronously** when data arrives on network
- Called before data is stored in assembly instance
- Called for **consuming** assemblies (data coming FROM scanner)

**Why it's critical**:
- **Immediate Processing**: Process control data as soon as it arrives
- **Validation**: Validate received data before using it
- **Real-Time Response**: Enables deterministic control response

**Parameters**:
- `instance`: Pointer to assembly instance that received data

**Returns**:
- `kEipStatusOk` (0): Data is valid and processed successfully
- `kEipStatusError` (-1): Data is invalid (for configuration assemblies, this may reject the connection)

**Example**:
```c
EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
    // Determine which assembly received data
    EipUint32 assembly_id = instance->instance_number;
    
    switch (assembly_id) {
        case 150:  // Output Assembly (consuming from scanner)
            // Process control data immediately
            EipUint8 *data = instance->data;
            
            // Validate data length
            if (instance->data_length < 32) {
                OPENER_TRACE_ERR("Invalid data length for assembly 150\n");
                return kEipStatusError;
            }
            
            // Update outputs atomically
            write_digital_outputs(data, 32);
            
            // Log for debugging
            OPENER_TRACE_INFO("Received output data: %02X %02X ...\n", 
                             data[0], data[1]);
            
            return kEipStatusOk;
            
        case 151:  // Configuration Assembly
            // Validate configuration data
            if (!validate_configuration(instance->data, instance->data_length)) {
                OPENER_TRACE_ERR("Invalid configuration data\n");
                return kEipStatusError;  // Reject connection
            }
            
            // Apply configuration
            apply_configuration(instance->data);
            return kEipStatusOk;
            
        default:
            OPENER_TRACE_WARN("Unknown assembly: %d\n", assembly_id);
            return kEipStatusOk;  // Don't reject, just ignore
    }
}
```

**Critical Timing**: This callback executes in the network receive path. Keep it fast for real-time performance!

**ESP32 Considerations**:
- Called from OpENer task (Core 0)
- If hardware I/O access is slow, use a queue to defer to I/O task
- For time-critical control, consider direct hardware access here

#### `EipBool8 BeforeAssemblyDataSend(CipInstance *instance)`

**Purpose**: Called just before assembly data is sent to update the data with current values.

**When it's called**:
- Called **synchronously** when connection is about to send data
- Called for **producing** assemblies (data going TO scanner)
- Called for both cyclic and application-triggered connections
- Called from `ManageConnections()` or `TriggerConnections()`

**Why it's critical**:
- **Fresh Data**: Ensures data is current when sent
- **Data Preparation**: Last chance to update assembly data before transmission
- **Change Detection**: Return value helps scanner detect data changes via sequence counter

**Parameters**:
- `instance`: Pointer to assembly instance that will send data

**Returns**:
- `true` (1): Data has changed - sequence counter will be incremented
- `false` (0): Data has not changed - sequence counter not incremented

**Note**: Data is **always sent** regardless of return value. The return value only affects sequence counter incrementation, which helps the scanner detect data changes.

**Example**:
```c
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    EipUint32 assembly_id = instance->instance_number;
    
    switch (assembly_id) {
        case 100:  // Input Assembly (producing to scanner)
            // Update assembly data with current sensor values
            EipUint8 *data = instance->data;
            
            // Read sensors and pack into assembly data
            data[0] = read_digital_input(0);
            data[1] = read_digital_input(1);
            data[2] = read_analog_input(0) & 0xFF;
            data[3] = (read_analog_input(0) >> 8) & 0xFF;
            
            // Update status bits
            data[31] = get_device_status_byte();
            
            // Always send (data is ready)
            return true;
            
        case 200:  // Another input assembly
            // Only send if data has changed
            static EipUint8 last_data[32] = {0};
            EipUint8 current_data[32];
            
            read_sensor_data(current_data, 32);
            
            if (memcmp(current_data, last_data, 32) != 0) {
                memcpy(instance->data, current_data, 32);
                memcpy(last_data, current_data, 32);
                return true;  // Send updated data
            }
            
            return false;  // No change - sequence counter won't increment
            
        default:
            return true;  // Default: always send
    }
}
```

**Performance Critical**: Called at RPI rate for cyclic connections. Must be fast!

**Best Practices**:
- Read sensors here, not in `HandleApplication`
- Keep data preparation simple and fast
- Return `true` when data changes, `false` when unchanged (helps scanner detect changes)
- Don't perform blocking I/O operations
- **Important**: Data is always sent - return value only affects sequence counter

### Connection Event Callbacks

#### `void CheckIoConnectionEvent(unsigned int output_assembly_id, unsigned int input_assembly_id, IoConnectionEvent io_connection_event)`

**Purpose**: Notifies application of connection state changes (opened, closed, timed out).

**When it's called**:
- When connection is established (opened)
- When connection is closed
- When connection times out
- Called for I/O connections (implicit messaging)

**Why it's important**:
- **State Awareness**: Application knows when connections start/stop
- **Resource Management**: Allocate/free resources based on connection state
- **Safety**: Enter safe state when connections close

**Parameters**:
- `output_assembly_id`: Output assembly connection point
- `input_assembly_id`: Input assembly connection point  
- `io_connection_event`: Event type (see `IoConnectionEvent` enum)

**Event Types**:
```c
typedef enum {
    kIoConnectionEventOpened = 0,      // Connection opened
    kIoConnectionEventTimedOut,        // Connection timed out
    kIoConnectionEventClosed            // Connection closed normally
} IoConnectionEvent;
```

**Example**:
```c
void CheckIoConnectionEvent(unsigned int output_assembly_id,
                            unsigned int input_assembly_id,
                            IoConnectionEvent io_connection_event) {
    OPENER_TRACE_INFO("IO Connection Event: Output=%d, Input=%d, Event=%d\n",
                     output_assembly_id, input_assembly_id, io_connection_event);
    
    switch (io_connection_event) {
        case kIoConnectionEventOpened:
            // Connection established - prepare for data exchange
            OPENER_TRACE_INFO("Connection opened: %d -> %d\n",
                             output_assembly_id, input_assembly_id);
            
            // Enable I/O processing
            enable_io_processing();
            
            // Allocate connection-specific resources if needed
            allocate_connection_resources(output_assembly_id, input_assembly_id);
            break;
            
        case kIoConnectionEventTimedOut:
            // Connection timed out - scanner stopped sending
            OPENER_TRACE_WARN("Connection timed out: %d -> %d\n",
                             output_assembly_id, input_assembly_id);
            
            // Enter safe state for safety-critical applications
            enter_safe_state();
            
            // Free resources
            free_connection_resources(output_assembly_id, input_assembly_id);
            break;
            
        case kIoConnectionEventClosed:
            // Connection closed normally
            OPENER_TRACE_INFO("Connection closed: %d -> %d\n",
                             output_assembly_id, input_assembly_id);
            
            // Free connection resources
            free_connection_resources(output_assembly_id, input_assembly_id);
            
            // Optionally enter safe state
            // enter_safe_state();  // Uncomment if needed
            break;
    }
}
```

**Safety Considerations**:
- For safety-critical applications, enter safe state on timeout
- Don't assume connection is active after timeout
- Clean up resources promptly to prevent leaks

### Service Callback

#### `typedef EipStatus (*CipServiceFunction)(CipInstance *const instance, CipMessageRouterRequest *const message_router_request, CipMessageRouterResponse *const message_router_response, struct sockaddr *originator_address, const int encapsulation_session)`

**Purpose**: Handler for CIP service requests (Get, Set, Reset, etc.).

**How it works**: 
1. Scanner sends explicit message with service code
2. Message Router routes to appropriate object
3. Object calls registered service function
4. Service function processes request and fills response

**Why service-based**:
- **Standardized**: All CIP services follow same pattern
- **Extensible**: Easy to add custom services
- **Modular**: Each service is independent

**Parameters**:
- `instance`: CIP instance receiving service request
- `message_router_request`: Request data (service code, path, data)
- `message_router_response`: Response structure (status, data)
- `originator_address`: Network address of requester
- `encapsulation_session`: Session ID for response routing

**Returns**: `kEipStatusOk` on success

**Example**:
```c
EipStatus MyGetAttributeService(CipInstance *instance,
                                CipMessageRouterRequest *request,
                                CipMessageRouterResponse *response,
                                struct sockaddr *originator_address,
                                const int encapsulation_session) {
    // Get attribute number from request
    EipUint16 attr_num = request->request_path.attribute;
    
    // Handle different attributes
    switch (attr_num) {
        case 1:
            // Return attribute 1 value
            response->response_data[0] = my_attribute_1_value;
            response->response_data_size = 1;
            response->general_status = kCipGeneralStatusCodeSuccess;
            break;
            
        case 2:
            // Return attribute 2 value
            memcpy(response->response_data, &my_attribute_2_value, sizeof(my_attribute_2_value));
            response->response_data_size = sizeof(my_attribute_2_value);
            response->general_status = kCipGeneralStatusCodeSuccess;
            break;
            
        default:
            // Attribute not found
            response->general_status = kCipGeneralStatusCodeAttributeNotSupported;
            break;
    }
    
    return kEipStatusOk;
}
```

## Callback Registration and Implementation

### Required Callbacks

All of these callbacks **must** be implemented in your application. OpENer will call them, and if they're missing, you'll get linker errors or undefined behavior.

**Complete List of Required Callbacks**:

1. **`ApplicationInitialization()`** - Called once at startup
2. **`HandleApplication()`** - Called periodically
3. **`AfterAssemblyDataReceived()`** - Called when data received
4. **`BeforeAssemblyDataSend()`** - Called before data sent
5. **`CheckIoConnectionEvent()`** - Called on connection events

### Optional/Extended Callbacks

These are registered for specific objects or connections:

- **`CipServiceFunction`** - Registered per CIP class via `InsertService()`
- **`OpenConnectionFunction`** - Registered per connection type
- **`ConnectionSendDataFunction`** - Registered per connection
- **`ConnectionReceiveDataFunction`** - Registered per connection
- **`ConnectionCloseFunction`** - Registered per connection
- **`ConnectionTimeoutFunction`** - Registered per connection

### Implementation Pattern

```c
// Required callbacks - implement these in your application

EipStatus ApplicationInitialization(void) {
    // Create objects, initialize state
    return kEipStatusOk;
}

void HandleApplication(void) {
    // Periodic processing
}

EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
    // Process received data
    return kEipStatusOk;
}

EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    // Update data before sending
    return true;
}

void CheckIoConnectionEvent(unsigned int output_assembly_id,
                            unsigned int input_assembly_id,
                            IoConnectionEvent io_connection_event) {
    // Handle connection events
}

// Optional: Custom service handler
EipStatus MyCustomService(CipInstance *instance,
                          CipMessageRouterRequest *request,
                          CipMessageRouterResponse *response,
                          struct sockaddr *originator_address,
                          const int encapsulation_session) {
    // Handle custom service
    response->general_status = kCipGeneralStatusCodeSuccess;
    return kEipStatusOk;
}

// Register custom service in ApplicationInitialization()
void ApplicationInitialization(void) {
    // ... create objects ...
    
    // Register custom service
    CipClass *my_class = GetMyCustomClass();
    InsertService(my_class, 0x99, MyCustomService, "MyCustomService");
}
```

### Callback Execution Flow

Understanding when callbacks are called helps with debugging:

```
Startup:
  CipStackInit()
    └─> ApplicationInitialization()  [Once]

Normal Operation (every 1-10ms):
  ManageConnections()
    ├─> HandleApplication()  [First]
    ├─> Check timers
    ├─> For each cyclic connection:
    │   └─> BeforeAssemblyDataSend()  [If time to send]
    │       └─> Send data
    └─> Check for timeouts

Data Received:
  NetworkHandlerProcessOnce()
    └─> HandleReceivedConnectedData()
        └─> AfterAssemblyDataReceived()  [Immediately]
            └─> Process data

Connection Events:
  ConnectionManager
    ├─> Connection opened
    │   └─> CheckIoConnectionEvent(..., kIoConnectionEventOpened)
    ├─> Connection closed
    │   └─> CheckIoConnectionEvent(..., kIoConnectionEventClosed)
    └─> Connection timeout
        └─> CheckIoConnectionEvent(..., kIoConnectionEventTimedOut)
```

## Callback Troubleshooting

### Common Issues and Solutions

#### Issue: Callback Not Being Called

**Symptoms**: Expected callback never executes

**Possible Causes**:
1. Callback not implemented (linker error)
2. Object/connection not created
3. Wrong assembly ID checked
4. Connection not established

**Solutions**:
```c
// Add debug logging to verify callback is registered
void CheckIoConnectionEvent(...) {
    OPENER_TRACE_INFO("CheckIoConnectionEvent called!\n");  // Verify it's called
    // ... rest of implementation
}

// Verify assembly exists
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    if (instance == NULL) {
        OPENER_TRACE_ERR("Instance is NULL!\n");
        return false;
    }
    OPENER_TRACE_INFO("BeforeAssemblyDataSend: assembly %d\n", 
                     instance->instance_number);
    // ... rest of implementation
}
```

#### Issue: Data Not Updating

**Symptoms**: Scanner receives stale data

**Possible Causes**:
1. `BeforeAssemblyDataSend()` not updating data
2. Data updated in wrong callback
3. Assembly data pointer incorrect

**Solutions**:
```c
// WRONG: Updating in HandleApplication
void HandleApplication(void) {
    assembly->data[0] = read_sensor();  // Too early! Data overwritten later
}

// CORRECT: Update in BeforeAssemblyDataSend
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    instance->data[0] = read_sensor();  // Right before sending
    return true;
}
```

#### Issue: Received Data Not Processed

**Symptoms**: Outputs don't change when scanner sends data

**Possible Causes**:
1. `AfterAssemblyDataReceived()` returning error
2. Wrong assembly ID checked
3. Data validation failing

**Solutions**:
```c
EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
    OPENER_TRACE_INFO("Received data for assembly %d, length %d\n",
                     instance->instance_number, instance->data_length);
    
    // Add validation logging
    if (instance->data_length < expected_length) {
        OPENER_TRACE_ERR("Data too short: %d < %d\n",
                        instance->data_length, expected_length);
        return kEipStatusError;  // This prevents processing!
    }
    
    // Process data
    process_data(instance->data, instance->data_length);
    return kEipStatusOk;  // Must return kEipStatusOk for data to be accepted
}
```

#### Issue: Performance Problems

**Symptoms**: Missed RPI deadlines, jitter, timeouts

**Possible Causes**:
1. Callbacks taking too long
2. Blocking operations in callbacks
3. Heavy processing in time-critical callbacks

**Solutions**:
```c
// BAD: Blocking I/O in callback
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    // This blocks for 100ms - too slow!
    read_slow_sensor();  
    return true;
}

// GOOD: Fast callback, defer heavy work
static volatile EipBool8 sensor_ready = false;
static EipUint8 cached_sensor_value = 0;

// Called from separate task
void sensor_task(void *arg) {
    while (1) {
        cached_sensor_value = read_slow_sensor();  // Block here, not in callback
        sensor_ready = true;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Fast callback
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    if (sensor_ready) {
        instance->data[0] = cached_sensor_value;  // Fast copy
        return true;
    }
    return false;  // Skip if not ready
}
```

#### Issue: Connection Events Not Handled

**Symptoms**: Resources not cleaned up, safe state not entered

**Possible Causes**:
1. `CheckIoConnectionEvent()` not implemented
2. Wrong event type checked
3. Event handler incomplete

**Solutions**:
```c
// Always handle all event types
void CheckIoConnectionEvent(unsigned int output_assembly_id,
                            unsigned int input_assembly_id,
                            IoConnectionEvent io_connection_event) {
    // Log all events for debugging
    const char *event_names[] = {
        "Opened", "TimedOut", "Closed"
    };
    OPENER_TRACE_INFO("Event: %s (Output=%d, Input=%d)\n",
                     event_names[io_connection_event],
                     output_assembly_id, input_assembly_id);
    
    // Handle each event type
    switch (io_connection_event) {
        case kIoConnectionEventOpened:
            // Always implement
            break;
        case kIoConnectionEventTimedOut:
            // Critical for safety
            break;
        case kIoConnectionEventClosed:
            // Always implement cleanup
            break;
        default:
            // Should not happen, but handle gracefully
            OPENER_TRACE_WARN("Unknown connection event: %d\n", io_connection_event);
            break;
    }
}
```

### Debugging Tips

1. **Add Logging**: Log entry/exit of all callbacks
2. **Check Return Values**: Verify callbacks return correct values
3. **Validate Parameters**: Check pointers and values in callbacks
4. **Measure Execution Time**: Profile callback execution time
5. **Use Breakpoints**: Set breakpoints in callbacks to trace execution

### ESP32-Specific Callback Considerations

1. **Task Context**: Callbacks run in OpENer task (Core 0)
2. **Stack Size**: Ensure OpENer task has enough stack for callbacks
3. **Interrupt Safety**: Don't call FreeRTOS blocking functions from callbacks
4. **Memory**: Use static/global variables if needed (callbacks are re-entrant)
5. **Priority**: OpENer task priority affects callback timing

## Best Practices

### 1. Initialization Order

```c
// 1. Configure stack parameters
ConfigureExplicitMessaging(4);

// 2. Initialize CIP objects (Identity, Message Router, etc.)
CipIdentityInit();
CipMessageRouterInit();
ConnectionManagerInit();

// 3. Create custom objects
CreateMyCustomObject();

// 4. Set device identity
SetDeviceSerialNumber(unique_serial);
SetDeviceStatus(0x0001);  // Configured

// 5. Start network handler
NetworkHandlerInitialize();
```

### 2. Main Loop Structure

```c
void main_loop(void) {
    MilliSeconds last_tick = GetSystemTimeMs();
    
    while (running) {
        MilliSeconds now = GetSystemTimeMs();
        MilliSeconds elapsed = now - last_tick;
        
        // 1. Process network messages (highest priority)
        NetworkHandlerProcessOnce();
        
        // 2. Manage connections (time-critical)
        ManageConnections(elapsed);
        
        // 3. Handle application logic
        HandleApplication();
        
        // 4. Update last tick time
        last_tick = now;
        
        // 5. Small delay to prevent CPU spinning
        if (elapsed < kOpenerTimerTickInMilliSeconds) {
            DelayMs(kOpenerTimerTickInMilliSeconds - elapsed);
        }
    }
}
```

### 3. Error Handling

```c
EipStatus result = SomeOpenerFunction();
if (result != kEipStatusOk) {
    // Log error with context
    log_error("OpENer function failed: %d", result);
    
    // Handle error appropriately
    // Don't ignore errors - they indicate real problems
    handle_error(result);
}
```

### 4. Memory Management

- **Stack Memory**: OpENer manages its own memory
- **Application Memory**: Application must manage callback-allocated memory
- **Attribute Data**: Keep attribute data pointers valid for object lifetime
- **Connection Data**: Allocate in `OpenConnectionFunction`, free in `CloseConnectionFunction`

### 5. Thread Safety

- OpENer is **not thread-safe** by default
- Call OpENer functions from single thread
- Use mutexes if multiple threads access OpENer
- Network handler typically runs in dedicated thread

## Common Patterns

### Pattern 1: Simple I/O Device

```c
// Initialize device
void device_init(void) {
    // Configure stack
    ConfigureExplicitMessaging(2);
    
    // Initialize standard objects
    CipIdentityInit();
    CipMessageRouterInit();
    ConnectionManagerInit();
    
    // Create I/O assembly
    CreateAssemblyObject(100, 32, kEipBool8Array);  // Input assembly
    CreateAssemblyObject(150, 32, kEipBool8Array);  // Output assembly
    
    // Set identity
    SetDeviceSerialNumber(0x12345678);
    SetDeviceStatus(0x0001);
    
    // Start network
    NetworkHandlerInitialize();
}

// Send data callback
EipStatus SendIOData(CipConnectionObject *conn) {
    CipInstance *assembly = GetAssemblyInstance(100);
    read_digital_inputs(assembly->data, 32);
    return kEipStatusOk;
}

// Receive data callback
EipStatus ReceiveIOData(CipConnectionObject *conn, 
                       const EipUint8 *data, 
                       EipUint16 length) {
    write_digital_outputs(data, length);
    return kEipStatusOk;
}
```

### Pattern 2: Custom CIP Object

```c
// Create custom object class
CipClass *CreateMyCustomObject(void) {
    CipClass *custom_class = CreateCipClass(0x99, "MyCustomObject");
    
    // Add attributes
    CipInstance *instance = CreateCipInstance(custom_class, 1);
    EipUint32 value = 0;
    InsertAttribute(instance, 1, kCipUdint, &value, 
                    kGetableSingle | kGetableAll | kSetable);
    
    // Add services
    InsertService(custom_class, kGetAttributeSingle, 
                 MyGetAttributeService, "Get");
    InsertService(custom_class, kSetAttributeSingle, 
                 MySetAttributeService, "Set");
    
    return custom_class;
}
```

## Callback Summary and Quick Reference

### Callback Priority and Timing

**Critical Timing (Called Frequently)**:
- `HandleApplication()` - Every 1-10ms
- `BeforeAssemblyDataSend()` - At RPI rate (can be 1-100ms)
- `AfterAssemblyDataReceived()` - When data arrives (asynchronous)

**Event-Driven (Called on Events)**:
- `ApplicationInitialization()` - Once at startup
- `CheckIoConnectionEvent()` - On connection state changes

### Callback Checklist

Use this checklist to ensure all callbacks are properly implemented:

- [ ] `ApplicationInitialization()` - Creates all CIP objects
- [ ] `HandleApplication()` - Performs periodic processing
- [ ] `AfterAssemblyDataReceived()` - Processes received control data
- [ ] `BeforeAssemblyDataSend()` - Updates sensor data before sending
- [ ] `CheckIoConnectionEvent()` - Handles connection lifecycle
- [ ] Custom service handlers (if needed) - Registered via `InsertService()`

### Key Takeaways

1. **Callbacks are the primary interface** - They're how OpENer communicates with your application
2. **Timing is critical** - Keep callbacks fast, especially `BeforeAssemblyDataSend` and `AfterAssemblyDataReceived`
3. **Return values matter** - `kEipStatusOk`/`kEipStatusError` and `true`/`false` control behavior
4. **Event-driven design** - Callbacks enable real-time, deterministic behavior
5. **Separation of concerns** - Stack handles protocol, callbacks handle application logic

### Common Patterns

**Pattern 1: Fast Sensor Reading**
```c
// Read sensors in BeforeAssemblyDataSend (fast, just before sending)
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    instance->data[0] = read_fast_sensor();
    return true;
}
```

**Pattern 2: Slow Sensor with Caching**
```c
// Cache slow sensor in separate task, use cache in callback
static volatile EipUint16 cached_temperature = 0;

EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    instance->data[0] = cached_temperature & 0xFF;
    instance->data[1] = (cached_temperature >> 8) & 0xFF;
    return true;
}
```

**Pattern 3: Immediate Output Update**
```c
// Update outputs immediately when data received
EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
    if (instance->instance_number == 150) {  // Output assembly
        write_outputs(instance->data, instance->data_length);
    }
    return kEipStatusOk;
}
```

**Pattern 4: Safe State on Timeout**
```c
// Enter safe state when connection lost
void CheckIoConnectionEvent(unsigned int output_assembly_id,
                            unsigned int input_assembly_id,
                            IoConnectionEvent io_connection_event) {
    if (io_connection_event == kIoConnectionEventTimedOut) {
        enter_safe_state();  // Critical for safety
    }
}
```

## Conclusion

The OpENer CIP API provides a powerful yet flexible interface for implementing EtherNet/IP devices. Key principles:

1. **Callback-Based**: Stack notifies application of events through callbacks
2. **Object-Oriented**: CIP objects encapsulate functionality
3. **Event-Driven**: Real-time performance through timely callbacks
4. **Modular**: Easy to extend with custom objects and services

**Callbacks are the heart of OpENer integration**. Understanding when and why each callback is called, implementing them correctly, and following best practices will help you create robust, efficient EtherNet/IP devices.

### Remember

- **Callbacks execute in OpENer task context** (Core 0 on ESP32)
- **Keep callbacks fast** - especially time-critical ones
- **Return correct values** - they control stack behavior
- **Handle all events** - don't ignore connection events
- **Validate data** - check lengths and values in callbacks
- **Log for debugging** - callbacks are hard to debug without logging

## Additional Resources

- [OpENer GitHub Repository](https://github.com/EIPStackGroup/OpENer)
- [OpENer API Documentation](https://eipstackgroup.github.io/OpENer/)
- [EtherNet/IP Specification](https://www.odva.org/)
- [CIP Specification](https://www.odva.org/)

---

*Last Updated: Based on OpENer API documentation*
*Project: ESP32-P4 EtherNet/IP Implementation*

