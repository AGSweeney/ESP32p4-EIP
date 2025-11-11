# ACD ARP Frame Storage Issue

## Summary

EtherNet/IP specification requires that the TCP/IP Interface Object store the precise ARP frame that triggered an address conflict. While the infrastructure exists to store this data, the ARP frame information is not currently captured when conflicts are detected because the ACD callback mechanism does not propagate this data.

## EtherNet/IP Requirement

**TCP/IP Interface Object (Class 0xF5), Attribute #11: "Last Conflict Detected"**

According to the EtherNet/IP specification, when an Address Conflict Detection (ACD) conflict occurs, the device must store:

1. **Activity Status** (1 byte): The conflict activity state
2. **Remote MAC Address** (6 bytes): The MAC address of the conflicting device
3. **Raw ARP Frame Data** (28 bytes): The complete ARP frame that triggered the conflict

This data must be accessible via the TCP/IP Interface Object Attribute #11 for diagnostic and troubleshooting purposes.

## Current Implementation Status

### ✅ What Exists

1. **Storage Structure** (`components/opener/src/cip/ciptcpipinterface.c`):
   ```c
   static struct {
     CipUsint activity;
     CipUsint remote_mac[6];
     CipUsint raw_data[28];
   } s_tcpip_last_conflict = {0};
   ```

2. **Storage Functions** (`components/opener/src/cip/ciptcpipinterface.h`):
   - `void CipTcpIpSetLastAcdMac(const uint8_t mac[6])`
   - `void CipTcpIpSetLastAcdRawData(const uint8_t *data, size_t length)`

3. **Encoding Function** (`components/opener/src/cip/ciptcpipinterface.c`):
   - `EncodeCipLastConflictDetected()` - Encodes the stored conflict data for Attribute #11

4. **Attribute Registration** (`components/opener/src/cip/ciptcpipinterface.c`, line 769-775):
   - Attribute #11 is registered and uses `EncodeCipLastConflictDetected` as its encoder

### ❌ What's Missing

1. **ACD Callback Limitation**:
   - The ACD callback signature (`acd_conflict_callback_t`) only passes:
     ```c
     typedef void (*acd_conflict_callback_t)(struct netif *netif, acd_callback_enum_t state);
     ```
   - **No ARP frame data is passed** to the callback

2. **Missing Function Calls**:
   - `CipTcpIpSetLastAcdMac()` is **never called** from conflict detection code
   - `CipTcpIpSetLastAcdRawData()` is **never called** from conflict detection code

3. **Available but Unused Data**:
   - In `acd_arp_reply()` (`dependency_modifications/lwIP/acd.c`), when conflicts are detected:
     - `hdr->shwaddr` (conflicting MAC address) is available
     - The complete ARP packet (`struct etharp_hdr *hdr`) is available
     - This data is **not captured or stored**

## Problem Location

### Conflict Detection Points

1. **Probe Conflict** (`dependency_modifications/lwIP/acd.c`, lines 484-492):
   ```c
   if ((ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
        !eth_addr_eq(&netifaddr, &hdr->shwaddr)) ||
       (ip4_addr_isany_val(sipaddr) &&
        ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
        !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
     acd_log_mac("conflict detected", &hdr->shwaddr, &sipaddr, &dipaddr);
     LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                 ("acd_arp_reply(): Probe Conflict detected\n"));
     acd_restart(netif, acd);  // ❌ ARP frame data not captured here
   }
   ```

2. **Ongoing Conflict** (`dependency_modifications/lwIP/acd.c`, lines 503-507):
   ```c
   if (ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
       !eth_addr_eq(&netifaddr, &hdr->shwaddr)) {
     LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                 ("acd_arp_reply(): Conflicting ARP-Packet detected\n"));
     acd_handle_arp_conflict(netif, acd);  // ❌ ARP frame data not captured here
   }
   ```

### Current Callback Implementation

**File**: `main/main.c`, lines 130-168

```c
static void tcpip_acd_conflict_callback(struct netif *netif, acd_callback_enum_t state) {
    ESP_LOGI(TAG, "ACD callback state=%d", (int)state);
    s_acd_last_state = state;
    switch (state) {
        case ACD_DECLINE:
        case ACD_RESTART_CLIENT:
            g_tcpip.status |= kTcpipStatusAcdStatus;
            g_tcpip.status |= kTcpipStatusAcdFault;
            CipTcpIpSetLastAcdActivity(3);  // ✅ Activity is set
            // ❌ But MAC address and raw ARP frame are NOT set
            break;
        // ...
    }
}
```

## Impact

1. **EtherNet/IP Compliance**: The device does not fully comply with the EtherNet/IP specification requirement for Attribute #11
2. **Diagnostic Capability**: Network administrators cannot retrieve the conflicting device's MAC address or ARP frame details via CIP Get_Attribute_Single
3. **Troubleshooting**: Limited diagnostic information available when conflicts occur

## Proposed Solution

### Option 1: Capture Data in ACD Module (Recommended)

Modify `dependency_modifications/lwIP/acd.c` to capture ARP frame data immediately when conflicts are detected, before calling the callback.

#### Changes Required

1. **Add Forward Declarations** (at top of `acd.c`):
   ```c
   /* Forward declarations for OpENer integration */
   extern void CipTcpIpSetLastAcdMac(const uint8_t mac[6]);
   extern void CipTcpIpSetLastAcdRawData(const uint8_t *data, size_t length);
   ```

2. **Modify `acd_restart()` Function** (around line 417):
   ```c
   static void
   acd_restart(struct netif *netif, struct acd *acd)
   {
     /* increase conflict counter. */
     acd->num_conflicts++;
   
     /* Decline the address */
     acd->acd_conflict_callback(netif, ACD_DECLINE);
     // Note: ARP frame data should be captured before this callback
     // However, hdr is not available in this function scope
     // See Option 2 for alternative approach
     
     // ... rest of function
   }
   ```

3. **Modify `acd_arp_reply()` Function** - Capture data at conflict detection points:

   **For Probe Conflicts** (around line 489):
   ```c
   if ((ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
        !eth_addr_eq(&netifaddr, &hdr->shwaddr)) ||
       (ip4_addr_isany_val(sipaddr) &&
        ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
        !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
     acd_log_mac("conflict detected", &hdr->shwaddr, &sipaddr, &dipaddr);
     LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                 ("acd_arp_reply(): Probe Conflict detected\n"));
     
     /* Capture ARP frame data for EtherNet/IP Attribute #11 */
     CipTcpIpSetLastAcdMac(hdr->shwaddr.addr);
     CipTcpIpSetLastAcdRawData((const uint8_t *)hdr, sizeof(struct etharp_hdr));
     
     acd_restart(netif, acd);
   }
   ```

   **For Ongoing Conflicts** (around line 503):
   ```c
   if (ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
       !eth_addr_eq(&netifaddr, &hdr->shwaddr)) {
     LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                 ("acd_arp_reply(): Conflicting ARP-Packet detected\n"));
     
     /* Capture ARP frame data for EtherNet/IP Attribute #11 */
     CipTcpIpSetLastAcdMac(hdr->shwaddr.addr);
     CipTcpIpSetLastAcdRawData((const uint8_t *)hdr, sizeof(struct etharp_hdr));
     
     acd_handle_arp_conflict(netif, acd);
   }
   ```

4. **Modify `acd_handle_arp_conflict()` Function** (around line 517):
   - Add `struct etharp_hdr *hdr` parameter to pass ARP frame data
   - Or store ARP frame data in `struct acd` structure temporarily

#### Considerations

- **Dependency**: This creates a dependency from LWIP ACD module to OpENer TCP/IP Interface object
- **Compilation**: Need to ensure `ciptcpipinterface.h` is accessible from `acd.c`
- **Linkage**: Functions must be linked correctly

### Option 2: Extend Callback Signature (Alternative)

Modify the ACD callback to include ARP frame data, requiring changes to both LWIP and application code.

#### Changes Required

1. **Modify Callback Type** (`dependency_modifications/lwIP/acd.c`):
   ```c
   typedef void (*acd_conflict_callback_t)(struct netif *netif, 
                                           acd_callback_enum_t state,
                                           const struct etharp_hdr *hdr);  // Add ARP header
   ```

2. **Update All Callback Invocations**:
   - `acd_restart()`: Pass `NULL` or stored ARP header
   - `acd_handle_arp_conflict()`: Pass `hdr` parameter
   - `acd_arp_reply()`: Pass `hdr` when calling conflict handlers

3. **Update Application Callback** (`main/main.c`):
   ```c
   static void tcpip_acd_conflict_callback(struct netif *netif, 
                                          acd_callback_enum_t state,
                                          const struct etharp_hdr *hdr) {
       // ... existing code ...
       
       if (hdr != NULL) {
           CipTcpIpSetLastAcdMac(hdr->shwaddr.addr);
           CipTcpIpSetLastAcdRawData((const uint8_t *)hdr, sizeof(struct etharp_hdr));
       }
   }
   ```

#### Considerations

- **Breaking Change**: Requires updating all callback implementations
- **Backward Compatibility**: May break existing code that uses ACD callbacks
- **More Invasive**: Requires changes to callback signature throughout codebase

### Option 3: Store in ACD Structure (Hybrid)

Store ARP frame data temporarily in the `struct acd` structure, then capture it in the callback.

#### Changes Required

1. **Extend `struct acd`** (`dependency_modifications/lwIP/acd.c`):
   ```c
   struct acd {
     // ... existing fields ...
     struct etharp_hdr last_conflict_arp;  // Store last conflict ARP frame
     bool has_conflict_data;                // Flag indicating data is valid
   };
   ```

2. **Store in Conflict Detection**:
   ```c
   // In acd_arp_reply(), when conflict detected:
   memcpy(&acd->last_conflict_arp, hdr, sizeof(struct etharp_hdr));
   acd->has_conflict_data = true;
   ```

3. **Retrieve in Callback**:
   ```c
   // In application callback, access via netif->acd_list
   // Extract ARP frame data and call CipTcpIpSetLastAcdMac/RawData
   ```

#### Considerations

- **Memory Overhead**: Adds ~28 bytes per ACD instance
- **Complexity**: Requires traversing ACD list in callback
- **Thread Safety**: Need to ensure data is valid when accessed

## Recommendation

**Option 1 (Capture Data in ACD Module)** is recommended because:

1. ✅ **Minimal Changes**: Only requires modifications to `acd.c`
2. ✅ **No Breaking Changes**: Callback signature remains unchanged
3. ✅ **Immediate Capture**: Data is captured at the exact point of conflict detection
4. ✅ **Clear Separation**: ACD module handles EtherNet/IP requirements directly

### Implementation Steps

1. Add `#include "ciptcpipinterface.h"` or forward declarations to `acd.c`
2. Add function calls to capture MAC and raw ARP data at conflict detection points
3. Test with actual conflict scenarios
4. Verify Attribute #11 returns correct data via CIP Get_Attribute_Single

### Testing Requirements

1. **Test Case 1**: Trigger conflict during probe phase
   - Verify `CipTcpIpSetLastAcdMac()` is called with conflicting MAC
   - Verify `CipTcpIpSetLastAcdRawData()` is called with ARP frame
   - Verify Attribute #11 returns correct MAC address

2. **Test Case 2**: Trigger conflict during ongoing phase
   - Verify same behavior as Test Case 1

3. **Test Case 3**: Multiple conflicts
   - Verify each conflict updates the stored data correctly

4. **Test Case 4**: CIP Get_Attribute_Single on Attribute #11
   - Verify returned data matches stored conflict information

## References

- **EtherNet/IP Specification**: TCP/IP Interface Object (Class 0xF5), Attribute #11
- **RFC 5227**: IPv4 Address Conflict Detection
- **Current Implementation**: 
  - `dependency_modifications/lwIP/acd.c` - ACD conflict detection
  - `components/opener/src/cip/ciptcpipinterface.c` - TCP/IP Interface Object
  - `main/main.c` - ACD callback implementation

## Status

- **Issue Identified**: ✅
- **Root Cause Analyzed**: ✅
- **Solution Proposed**: ✅
- **Implementation**: ⏳ Pending
- **Testing**: ⏳ Pending

---

*Last Updated: [Current Date]*
*Issue Status: Documented - Awaiting Implementation*

