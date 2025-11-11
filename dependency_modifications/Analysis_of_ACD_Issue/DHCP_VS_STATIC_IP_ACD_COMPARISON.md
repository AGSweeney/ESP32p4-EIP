# DHCP vs Static IP ACD Comparison

## Overview

This document compares how Address Conflict Detection (ACD) works for DHCP and Static IP configurations during interface bring-up in LWIP 5.5.1.

## Critical Difference: Timing of IP Assignment

### DHCP: ACD BEFORE IP Assignment ✅

```
1. dhcp_start(netif)
   └─ acd_add(netif, &dhcp->acd, dhcp_conflict_callback)
      └─ Registers callback (but ACD not started yet)

2. DHCP Protocol Flow:
   ├─ DISCOVER → OFFER → REQUEST → ACK
   └─ When ACK received:
      └─ dhcp_check(netif) called
         ├─ Sets DHCP_STATE_CHECKING
         └─ acd_start(netif, &dhcp->acd, dhcp->offered_ip_addr)
            └─ **IP NOT ASSIGNED YET** ✅

3. ACD Probing (2 ARP probes, ~1 second total)
   └─ Port-specific implementation (port/acd_dhcp_check.c)
      ├─ Send ARP probe #1
      ├─ Wait 500ms timeout
      ├─ Send ARP probe #2  
      └─ Wait 500ms timeout

4. ACD Callback:
   ├─ ACD_IP_OK → dhcp_bind(netif)
   │  └─ **NOW assigns IP address** ✅
   │     └─ netif_set_addr(netif, &ip, &netmask, &gw)
   │
   ├─ ACD_DECLINE → dhcp_decline(netif)
   │  └─ Removes IP, sends DECLINE to server
   │
   └─ ACD_RESTART_CLIENT → Wait 10s, restart discovery
```

**Key Point**: In DHCP, the IP address is **NOT assigned to the interface** until ACD confirms it's safe (`ACD_IP_OK` callback). This prevents routing with a conflicting address.

### Static IP: ACD AFTER IP Assignment ⚠️

**Without Fix (Original):**
```
1. netif_set_addr(netif, &ip, &netmask, &gw)
   └─ netif_do_set_ipaddr()
      ├─ **IP ASSIGNED IMMEDIATELY** ⚠️
      ├─ Routing enabled immediately
      ├─ netif_issue_reports() may send ARP
      └─ acd_netif_ip_addr_changed() called
         └─ **DOES NOTHING** (exits early) ❌

2. Application must manually:
   ├─ acd_add(netif, &my_acd, my_callback)
   └─ acd_start(netif, &my_acd, ip)
      └─ **ACD starts AFTER IP already assigned** ⚠️
```

**With Fix (LWIP_ACD_AUTO_START_STATIC=1):**
```
1. netif_set_addr(netif, &ip, &netmask, &gw)
   └─ netif_do_set_ipaddr()
      ├─ **IP ASSIGNED IMMEDIATELY** ⚠️
      ├─ Routing enabled immediately
      ├─ netif_issue_reports() may send ARP
      └─ acd_netif_ip_addr_changed() called
         └─ Detects new static IP
            ├─ acd_add(netif, static_acd, default_callback)
            └─ acd_start(netif, static_acd, ip)
               └─ **ACD starts AFTER IP already assigned** ⚠️

2. ACD Probing (Full RFC 5227 implementation)
   ├─ PROBE_WAIT (random 0-1s)
   ├─ PROBING (3 ARP probes, ~2-4s)
   ├─ ANNOUNCE_WAIT (2s)
   ├─ ANNOUNCING (2 ARP announcements, ~2s)
   └─ ONGOING (conflict detection continues)
      └─ Total: ~6-9 seconds

3. ACD Callback:
   ├─ ACD_IP_OK → Logged (no action, IP already assigned)
   └─ ACD_DECLINE/RESTART → Logged (IP still assigned!) ⚠️
```

**Key Point**: Even with the fix, the IP address is **assigned BEFORE ACD completes**. There's a race condition window where the interface may route with a conflicting address.

## Detailed Comparison

| Aspect | DHCP | Static IP (Original) | Static IP (With Fix) |
|--------|------|---------------------|---------------------|
| **ACD Start Timing** | Before IP assignment | Manual (after IP) | Automatic (after IP) |
| **IP Assignment Timing** | After ACD confirms | Before ACD starts | Before ACD starts |
| **ACD Implementation** | Port-specific (`acd_dhcp_check.c`) | Core (`acd.c`) | Core (`acd.c`) |
| **Probe Count** | 2 probes | 3 probes | 3 probes |
| **Probe Method** | `etharp_query()` with timeout | `etharp_acd_probe()` | `etharp_acd_probe()` |
| **Timeout Mechanism** | `sys_timeout()` callbacks | `acd_tmr()` periodic timer | `acd_tmr()` periodic timer |
| **Total ACD Duration** | ~1 second | ~6-9 seconds | ~6-9 seconds |
| **State Machine** | Simple (PROBING only) | Full RFC 5227 | Full RFC 5227 |
| **Conflict Handling** | DHCP-specific callbacks | Generic callbacks | Generic callbacks |
| **IP Removal on Conflict** | Yes (via `dhcp_decline()`) | No (manual) | No (logged only) |
| **Automatic Restart** | Yes (10s delay) | No | No |

## Code Flow Comparison

### DHCP ACD Flow

```c
// In dhcp.c:

dhcp_start(netif)
  └─ acd_add(netif, &dhcp->acd, dhcp_conflict_callback)  // Line 927

// ... DHCP protocol exchange ...

dhcp_recv(netif, p)
  └─ dhcp_handle_ack(netif, msg_in)
      └─ dhcp_check(netif)  // Line 416
          └─ acd_start(netif, &dhcp->acd, dhcp->offered_ip_addr)  // Line 425
              └─ **IP NOT ASSIGNED YET**

// Port-specific ACD (port/acd_dhcp_check.c):
acd_start() → send_probe_once() → etharp_query() → timeout callback
  └─ After 2 probes (~1s):
      └─ dhcp_conflict_callback(netif, ACD_IP_OK)
          └─ dhcp_bind(netif)  // **NOW assigns IP**
              └─ netif_set_addr(netif, &ip, &netmask, &gw)
```

### Static IP ACD Flow (With Fix)

```c
// In netif.c:

netif_set_addr(netif, &ip, &netmask, &gw)
  └─ netif_do_set_ipaddr(netif, ipaddr, &old_addr)
      ├─ ip4_addr_set(ip_2_ip4(&netif->ip_addr), ipaddr)  // **IP ASSIGNED**
      ├─ mib2_add_ip4(netif)  // Routing enabled
      ├─ netif_issue_reports()  // May send ARP
      └─ acd_netif_ip_addr_changed(netif, old_addr, &new_addr)
          └─ Detects new static IP
              ├─ acd_add(netif, static_acd, default_callback)
              └─ acd_start(netif, static_acd, ip)  // **ACD starts AFTER IP**

// Core ACD (lwip/src/core/ipv4/acd.c):
acd_start() → Sets ACD_STATE_PROBE_WAIT
  └─ acd_tmr() called periodically (every 100ms)
      └─ State machine: PROBE_WAIT → PROBING → ANNOUNCE_WAIT → ANNOUNCING → ONGOING
          └─ After ~6-9 seconds:
              └─ acd_conflict_callback(netif, ACD_IP_OK)
                  └─ **IP already assigned, just logs**
```

## Key Differences Summary

### 1. **IP Assignment Timing** (Most Critical)

- **DHCP**: IP assigned **AFTER** ACD confirms safety
- **Static IP**: IP assigned **BEFORE** ACD starts (even with fix)

**Impact**: Static IP has a race condition window where conflicting traffic may be routed.

### 2. **ACD Implementation**

- **DHCP**: Uses port-specific simplified implementation (`port/acd_dhcp_check.c`)
  - 2 ARP probes with timeout callbacks
  - ~1 second total duration
  - Simple state machine (PROBING only)

- **Static IP**: Uses full RFC 5227 implementation (`lwip/src/core/ipv4/acd.c`)
  - 3 ARP probes + 2 announcements
  - ~6-9 seconds total duration
  - Full state machine (PROBE_WAIT → PROBING → ANNOUNCE_WAIT → ANNOUNCING → ONGOING)

### 3. **Conflict Handling**

- **DHCP**: 
  - `ACD_DECLINE` → Removes IP, sends DECLINE to server
  - `ACD_RESTART_CLIENT` → Waits 10s, restarts discovery
  - `ACD_IP_OK` → Assigns IP

- **Static IP**:
  - `ACD_DECLINE` → Logged only (IP remains assigned)
  - `ACD_RESTART_CLIENT` → Logged only (IP remains assigned)
  - `ACD_IP_OK` → Logged only (IP already assigned)

### 4. **Automatic Start**

- **DHCP**: Always automatic (integrated into DHCP flow)
- **Static IP (Original)**: Manual (application must call `acd_add()` and `acd_start()`)
- **Static IP (With Fix)**: Automatic when `LWIP_ACD_AUTO_START_STATIC=1`

## Why DHCP Implementation is Better

1. **No Race Condition**: IP is not assigned until ACD confirms it's safe
2. **Proper Conflict Handling**: IP is removed on conflict, DHCP server notified
3. **Faster**: Simplified implementation completes in ~1 second vs ~6-9 seconds
4. **Integrated**: Seamlessly integrated into DHCP protocol flow

## Why Static IP Implementation Differs

1. **Architectural Limitation**: `netif_set_addr()` assigns IP immediately, before ACD can start
2. **Full RFC Compliance**: Uses complete RFC 5227 implementation for ongoing conflict detection
3. **No Automatic Removal**: Static IPs are "static" - the application must decide what to do on conflict
4. **Application Control**: Applications may want to handle conflicts differently (e.g., alert user, use fallback IP)

## Recommendations

### For Static IP with ACD:

1. **Use the Fix**: Enable `LWIP_ACD_AUTO_START_STATIC=1` to automatically start ACD
2. **Provide Custom Callback**: Implement your own conflict callback to handle conflicts appropriately:
   ```c
   void my_static_ip_callback(struct netif *netif, acd_callback_enum_t state)
   {
       switch (state) {
           case ACD_DECLINE:
           case ACD_RESTART_CLIENT:
               // Remove IP, alert user, use fallback, etc.
               netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
               break;
       }
   }
   ```

3. **Consider Timing**: Be aware that IP is assigned before ACD completes. If conflicts are critical, consider:
   - Waiting for `ACD_IP_OK` before using the IP for critical operations
   - Implementing application-level conflict handling
   - Using DHCP instead if possible

### Ideal Solution (Future Enhancement):

To fully match DHCP behavior, LWIP would need:
- A new API: `netif_set_addr_with_acd()` that defers IP assignment until ACD completes
- Or modify `netif_set_addr()` to accept an optional callback and defer assignment

This would require significant architectural changes to LWIP's IP assignment flow.

## Conclusion

**No, DHCP and Static IP do NOT perform ACD in a similar manner:**

1. **Timing**: DHCP runs ACD **before** IP assignment; Static IP assigns IP **before** ACD starts
2. **Implementation**: DHCP uses simplified port-specific code; Static IP uses full RFC 5227 implementation
3. **Duration**: DHCP completes in ~1s; Static IP takes ~6-9s
4. **Conflict Handling**: DHCP removes IP on conflict; Static IP logs conflicts but IP remains assigned

The fix I implemented makes Static IP ACD **automatic** (like DHCP), but it cannot fix the fundamental timing issue without major architectural changes to LWIP.

