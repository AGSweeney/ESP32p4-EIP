# ACD Implementation Comparison: LWIP-STACK vs ESP-IDF_LWIP

## Executive Summary

This document compares the Address Conflict Detection (ACD) implementation between the **LWIP-STACK** and **ESP-IDF_LWIP** codebases. The ESP-IDF_LWIP version includes significant enhancements for RFC 5227 compliance, particularly for static IP assignment, and includes a critical bug fix for self-conflict detection.

---

## Key Differences

### 1. RFC 5227 Compliant Static IP Support

**ESP-IDF_LWIP**: ✅ **Supported**
- Implements `LWIP_ACD_RFC5227_COMPLIANT_STATIC` feature
- Defers static IP assignment until ACD confirms the address is safe
- Uses `netif_pending_ip_config` structure to hold pending configuration
- Provides `acd_static_ip_rfc5227_callback()` function
- Includes `netif_set_addr_with_acd()` API in `netif.h`

**LWIP-STACK**: ❌ **Not Supported**
- No RFC 5227 compliant static IP feature
- Static IP addresses are assigned immediately without ACD verification

**Impact**: ESP-IDF_LWIP ensures static IP addresses are properly validated before assignment, preventing conflicts. LWIP-STACK may assign conflicting static IPs.

---

### 2. Critical Bug Fix: Self-Conflict Detection

**ESP-IDF_LWIP**: ✅ **Fixed**

In `acd_arp_reply()` during PROBE/PROBE_WAIT states (lines 419-423):
```c
if ((ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr)) ||  // ✅ Checks MAC address
    (ip4_addr_isany_val(sipaddr) &&
     ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
```

**LWIP-STACK**: ❌ **Bug Present**

In `acd_arp_reply()` during PROBE/PROBE_WAIT states (lines 410-413):
```c
if ((ip4_addr_eq(&sipaddr, &acd->ipaddr)) ||  // ❌ Missing MAC check
    (ip4_addr_isany_val(sipaddr) &&
     ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
```

**Impact**: LWIP-STACK incorrectly treats its own ARP probes as conflicts, causing infinite probe restarts. ESP-IDF_LWIP correctly ignores its own packets per RFC 5227 Section 2.2.1.

**Reference**: See `ESP-IDF_LWIP/lwip/docs/acd-static-ip-issue.md` for detailed explanation.

---

### 3. Protocol Constants Configuration

**ESP-IDF_LWIP**: ✅ **Configurable**

In `prot/acd.h` (lines 45-90):
```c
/* These can be overridden in lwipopts.h for protocol-specific requirements */
#ifndef PROBE_WAIT
#define PROBE_WAIT           1
#endif
#ifndef PROBE_MIN
#define PROBE_MIN            1
#endif
// ... all constants are conditionally defined
```

**LWIP-STACK**: ❌ **Hardcoded**

In `prot/acd.h` (lines 45-55):
```c
#define PROBE_WAIT           1   /* Hardcoded, cannot override */
#define PROBE_MIN            1
#define PROBE_MAX            2
// ... all constants are hardcoded
```

**Impact**: ESP-IDF_LWIP allows customization for different protocols (e.g., EtherNet/IP uses different timings). LWIP-STACK requires code modification to change timings.

---

### 4. Header Dependencies and Structure

**ESP-IDF_LWIP**: More Complex Dependencies

- `acd.h` includes forward declarations to avoid circular dependencies
- Uses `netif_pending_ip.h` for RFC 5227 static IP support
- More careful header organization to support conditional compilation

**LWIP-STACK**: Simpler Dependencies

- Direct includes: `#include "lwip/etharp.h"` in `acd.h`
- No `netif_pending_ip.h` file
- Simpler structure but less flexible

**Impact**: ESP-IDF_LWIP's structure supports more features but is more complex. LWIP-STACK is simpler but less extensible.

---

### 5. DHCP Integration

**ESP-IDF_LWIP**: ✅ **Dual-Mode Support**

ESP-IDF_LWIP supports two modes for DHCP conflict detection:

1. **`DHCP_DOES_ARP_CHECK`** (Fast Mode):
   - Uses custom port file: `ESP-IDF_LWIP/lwip/port/acd_dhcp_check.c`
   - Timeout-based approach (`ACD_DHCP_ARP_REPLY_TIMEOUT_MS = 500ms`)
   - Sends single ARP query and waits for timeout
   - Faster (1-2 seconds) but less thorough
   - Integrates with DHCP state machine (`DHCP_STATE_CHECKING`)

2. **`LWIP_DHCP_DOES_ACD_CHECK`** (RFC 5227 Compliant):
   - Uses standard ACD implementation
   - Full RFC 5227 probe/announce sequence
   - Slower (several seconds) but fully compliant

Configuration in `lwipopts.h`:
```c
#ifdef CONFIG_LWIP_DHCP_DOES_ARP_CHECK
#define DHCP_DOES_ARP_CHECK             1
#define LWIP_DHCP_DOES_ACD_CHECK        1
#elif CONFIG_LWIP_DHCP_DOES_ACD_CHECK
#define DHCP_DOES_ARP_CHECK             0
#define LWIP_DHCP_DOES_ACD_CHECK        1
```

**LWIP-STACK**: Uses Standard Implementation Only

- Only supports `LWIP_DHCP_DOES_ACD_CHECK`
- Uses the generic ACD implementation
- No custom DHCP port file
- Relies on standard ACD state machine
- No fast ARP-check mode option

**Impact**: ESP-IDF_LWIP provides flexibility to choose between fast ARP checking or full RFC 5227 compliance. LWIP-STACK only supports the slower but more thorough RFC 5227 method.

---

### 6. Function Return Types

**ESP-IDF_LWIP**:
- `acd_remove()`: Returns `void` (line 161)
- `acd_stop()`: Returns `err_t` (line 219)

**LWIP-STACK**:
- `acd_remove()`: Returns `void` (line 151)
- `acd_stop()`: Returns `err_t` (line 209)

**Impact**: Both are consistent, no functional difference.

---

### 7. Conflict Handling for Static IP

**ESP-IDF_LWIP**: ✅ **Enhanced Handling**

In `acd_handle_arp_conflict()` (lines 454-476):
```c
#if LWIP_ACD_RFC5227_COMPLIANT_STATIC
  if (netif->pending_ip_config != NULL && 
      ip4_addr_eq(&acd->ipaddr, &netif->pending_ip_config->ipaddr)) {
    /* Remove IP address */
    netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
    /* Notify user callback */
    if (netif->pending_ip_config->user_callback) {
      netif->pending_ip_config->user_callback(netif, ACD_DECLINE);
    }
    /* Clean up */
    acd_remove(netif, &netif->pending_ip_config->acd);
    mem_free(netif->pending_ip_config);
    netif->pending_ip_config = NULL;
    return;
  }
#endif
```

**LWIP-STACK**: Standard Handling Only

- No special handling for static IP conflicts
- Relies on standard conflict callback mechanism

**Impact**: ESP-IDF_LWIP properly cleans up pending static IP configurations on conflict, preventing resource leaks.

---

## Code Statistics

| Metric | LWIP-STACK | ESP-IDF_LWIP |
|--------|------------|--------------|
| `acd.c` Lines | 558 | 652 |
| `acd.h` Lines | 110 | 117 |
| `prot/acd.h` Lines | 92 | 127 |
| Additional Files | 0 | 2 (`acd_dhcp_check.c`, `netif_pending_ip.h`) |

---

## Recommendations

### For LWIP-STACK Users:

1. **Apply Self-Conflict Fix**: Update `acd_arp_reply()` to check MAC address in the first condition (line 410)
2. **Consider RFC 5227 Support**: Evaluate if RFC 5227 compliant static IP assignment is needed
3. **Make Constants Configurable**: Allow protocol-specific timing overrides

### For ESP-IDF_LWIP Users:

1. **Verify DHCP Integration**: Ensure custom DHCP port implementation meets requirements
2. **Test Static IP**: Validate RFC 5227 static IP feature works correctly in your environment
3. **Monitor Performance**: Custom DHCP implementation may have different timing characteristics

---

## Detailed Code Differences

### Conflict Detection Logic Comparison

**Location**: `acd_arp_reply()` function, PROBE/PROBE_WAIT case

**ESP-IDF_LWIP** (Fixed):
```c
if ((ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr)) ||  // ✅ MAC check prevents self-conflict
    (ip4_addr_isany_val(sipaddr) &&
     ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
```

**LWIP-STACK** (Buggy):
```c
if ((ip4_addr_eq(&sipaddr, &acd->ipaddr)) ||  // ❌ Missing MAC check causes false conflicts
    (ip4_addr_isany_val(sipaddr) &&
     ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
```

---

## Conclusion

The **ESP-IDF_LWIP** implementation is significantly more advanced with:
- ✅ RFC 5227 compliant static IP support
- ✅ Critical self-conflict detection bug fix
- ✅ Configurable protocol constants
- ✅ Enhanced conflict handling
- ✅ Custom DHCP integration

The **LWIP-STACK** implementation is simpler but has a critical bug that prevents proper ACD operation with static IP addresses. It should be updated with the self-conflict detection fix at minimum.

---

## References

- RFC 5227: IPv4 Address Conflict Detection
- ESP-IDF Documentation: `ESP-IDF_LWIP/lwip/docs/acd-static-ip-issue.md`
- ESP-IDF Port File: `ESP-IDF_LWIP/lwip/port/acd_dhcp_check.c`

