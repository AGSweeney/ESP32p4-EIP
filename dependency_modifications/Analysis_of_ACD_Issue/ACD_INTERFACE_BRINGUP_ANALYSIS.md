# Interface Bring-Up and ACD Process Analysis for ESP32 LWIP - Static IP Assignment

## Overview
This document describes the process of network interface bring-up and Address Conflict Detection (ACD) integration in LWIP on ESP32, with specific focus on **static IP assignment**. Several critical logic flaws are identified that affect static IP configurations.

## Process Flow for Static IP Assignment

### 1. Interface Initialization Sequence

```
1. netif_set_up(netif)
   ├─ Sets NETIF_FLAG_UP flag
   ├─ Triggers NETIF_STATUS_CALLBACK
   └─ Calls netif_issue_reports() (if link is up)

2. Application sets static IP
   └─ netif_set_addr(netif, &static_ip, &netmask, &gw)
      ├─ Calls netif_do_set_ipaddr()
      ├─ Triggers: acd_netif_ip_addr_changed(netif, old_addr, new_addr)
      │  └─ **PROBLEM**: Only handles link-local → routable transitions
      │     └─ Does NOT start ACD for new static IP assignments!
      ├─ Sets IP address: netif->ip_addr = static_ip
      └─ Calls netif_issue_reports()
         └─ May send gratuitous ARP immediately
```

### 2. Manual ACD Start Required [CRITICAL]

```
Application MUST manually start ACD [REQUIRED - NOT AUTOMATIC]
├─ Allocate ACD structure (typically static or on heap)
├─ Register callback: acd_add(netif, &my_acd, my_conflict_callback)
└─ Start ACD: acd_start(netif, &my_acd, static_ip)
   ├─ Sets ACD state: ACD_STATE_PROBE_WAIT
   ├─ Initializes: acd->sent_num = 0
   └─ Sets random wait timer: acd->ttw = ACD_RANDOM_PROBE_WAIT()
```

### 3. ACD Timer-Based Process (Static IP - Core Implementation)

```
1. acd_tmr() must be called periodically (every ACD_TMR_INTERVAL = 100ms)
   └─ This is typically done by the TCPIP thread

2. ACD State Machine (for static IP):
   
   PROBE_WAIT → PROBING → ANNOUNCE_WAIT → ANNOUNCING → ONGOING
   
   a) PROBE_WAIT:
      └─ Waits random time (0 to PROBE_WAIT seconds)
      └─ Then transitions to PROBING
   
   b) PROBING:
      └─ Sends ARP probe: etharp_acd_probe(netif, &acd->ipaddr)
      └─ Increments: acd->sent_num++
      └─ If acd->sent_num >= PROBE_NUM (typically 3):
         └─ Transition to ANNOUNCE_WAIT
      └─ Else: Wait random interval, send next probe
   
   c) ANNOUNCE_WAIT:
      └─ Waits ANNOUNCE_WAIT seconds
      └─ Then transitions to ANNOUNCING
   
   d) ANNOUNCING:
      └─ Sends ARP announcement: etharp_acd_announce(netif, &acd->ipaddr)
      └─ Increments: acd->sent_num++
      └─ If acd->sent_num >= ANNOUNCE_NUM (typically 2):
         └─ Transition to ONGOING
         └─ Call callback: acd_conflict_callback(netif, ACD_IP_OK)
```

### 4. ARP Conflict Detection (Static IP - Core ACD)

```
1. ARP Reply/Request Received
   └─ acd_arp_reply() [called from etharp_input()]
      ├─ Checks: acd->state (PROBE_WAIT, PROBING, ANNOUNCE_WAIT, etc.)
      ├─ Extracts: sender IP (sipaddr), sender MAC (shwaddr)
      └─ Conflict Detection Logic:
         
         During PROBE/PROBE_WAIT/ANNOUNCE_WAIT:
         ├─ Conflict if: sipaddr == acd->ipaddr AND shwaddr != netif->hwaddr
         └─ **FIXED**: Now checks MAC address (prevents self-conflict)
            └─ Calls: acd_restart(netif, acd)
               ├─ Calls callback: ACD_DECLINE
               └─ Calls callback: ACD_RESTART_CLIENT
         
         During ANNOUNCING/ONGOING:
         └─ Conflict if: sipaddr == acd->ipaddr AND shwaddr != netif->hwaddr
            └─ Calls: acd_handle_arp_conflict()
               └─ May put ACD in passive mode or restart
```

## Critical Flaws for Static IP Assignment

### Flaw #1: No Automatic ACD Start for Static IP

**Problem:**
When `netif_set_addr()` is called with a static IP address:
1. `acd_netif_ip_addr_changed()` is called
2. **BUT** this function only handles transitions from link-local to routable addresses
3. It does **NOT** automatically start ACD for new static IP assignments
4. The application must manually call `acd_add()` and `acd_start()`

**Location:**
- `lwip/src/core/netif.c:499` - `acd_netif_ip_addr_changed()` called
- `lwip/src/core/ipv4/acd.c:525-556` - `acd_netif_ip_addr_changed()` implementation

**Code Evidence:**
```c
void acd_netif_ip_addr_changed(struct netif *netif, const ip_addr_t *old_addr,
                                const ip_addr_t *new_addr)
{
  /* If we change from ANY to an IP or from an IP to ANY we do nothing */
  if (ip_addr_isany(old_addr) || ip_addr_isany(new_addr)) {
    return;  // ← EXITS HERE for new static IP!
  }
  
  // Only handles link-local → routable transitions
  if (ip_addr_islinklocal(old_addr) && !ip_addr_islinklocal(new_addr)) {
    acd_put_in_passive_mode(netif, acd);  // Only for existing ACD modules
  }
  // ← NO CODE TO START ACD FOR NEW STATIC IP!
}
```

**Impact:**
- **Static IP addresses are assigned WITHOUT conflict detection unless the application explicitly starts ACD**
- This violates the expectation that ACD would protect against conflicts
- Applications may not be aware they need to manually start ACD
- The interface may be brought up with a conflicting IP address

### Flaw #2: Self-Conflict Detection (Fixed in Current Code)

**Problem:**
The core ACD implementation (`lwip/src/core/ipv4/acd.c`) had a bug where:
1. Ethernet MAC reflects broadcast ARP probes back to the host
2. The ACD logic would see its own ARP probe and detect it as a conflict
3. This caused infinite restart loops for static IP addresses

**Status:**
- **FIXED** in current code (lines 410-414)
- Now checks MAC address: `!eth_addr_eq(&netifaddr, &hdr->shwaddr)`
- Prevents self-conflict detection

**Location:**
- `lwip/src/core/ipv4/acd.c:410-414` - Fixed conflict detection logic

**Fixed Code:**
```c
if ((ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr)) ||
    (ip4_addr_isany_val(sipaddr) &&
     ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
     !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
  // Conflict detected - MAC address differs from ours
  acd_restart(netif, acd);
}
```

### Flaw #3: Missing Integration Between netif_set_addr() and ACD

**Problem:**
The `netif_set_addr()` function:
1. Sets the IP address immediately
2. Calls `netif_issue_reports()` which may send gratuitous ARP
3. But does NOT wait for ACD to complete before using the address

**Sequence Issue:**
```
Time T0: netif_set_addr(netif, &static_ip, ...)
         ├─ IP address set: netif->ip_addr = static_ip
         ├─ Routing enabled: netif can route packets
         ├─ netif_issue_reports() may send ARP immediately
         └─ ACD NOT started yet

Time T1: Application calls acd_add() and acd_start()
         └─ ACD begins probing

Time T2-T3: ACD probes complete (several seconds later)
            └─ Callback: ACD_IP_OK or ACD_DECLINE
```

**During T0-T1:**
- Interface is routing with potentially conflicting address
- No conflict detection active
- If conflict exists, other hosts may see duplicate IP
- Gratuitous ARP may have already announced the conflicting address

**Impact:**
- The IP address is set and may be used for routing **before** ACD confirms it's safe
- If a conflict exists, the interface may already be routing traffic with a conflicting address
- Applications may start using the address before ACD completes
- Network may experience IP conflicts before ACD starts

### Flaw #4: No ACD State Persistence Across Interface Restart

**Problem:**
If the interface is brought down and up again:
1. `netif_set_down()` clears the IP address
2. `netif_set_up()` brings interface back up
3. `netif_set_addr()` sets IP again
4. **ACD state is lost** - must be restarted manually

**Impact:**
- Applications must track ACD state separately
- No automatic ACD restart on interface bring-up
- Risk of using conflicting address if ACD restart is forgotten
- Each interface restart requires manual ACD re-initialization

### Flaw #5: Race Condition Between Address Assignment and ACD Start

**Problem:**
For static IP, there's a timing window where the address is active but ACD hasn't started:

```
Time T0: netif_set_addr(netif, &static_ip, ...)
         ├─ IP address set: netif->ip_addr = static_ip
         ├─ Routing enabled: netif can route packets
         └─ ACD NOT started yet

Time T1: Application calls acd_add() and acd_start()
         └─ ACD begins probing

Time T2-T3: ACD probes complete
            └─ Callback: ACD_IP_OK or ACD_DECLINE
```

**During T0-T1:**
- Interface is routing with potentially conflicting address
- No conflict detection active
- If conflict exists, other hosts may see duplicate IP
- Other devices may cache incorrect ARP entries

**Impact:**
- Critical security/reliability issue
- Network may experience IP conflicts before ACD starts
- Other devices may cache incorrect ARP entries
- Traffic may be routed incorrectly during this window

## Comparison: DHCP vs Static IP ACD

| Aspect | DHCP | Static IP |
|--------|------|-----------|
| **ACD Start** | Automatic (via `dhcp_check()`) | **Manual** (application must call `acd_start()`) |
| **ACD Implementation** | Port-specific (`acd_dhcp_check.c`) | Core (`acd.c`) |
| **Timing** | Before IP assignment | **After** IP assignment |
| **State Machine** | Simple (PROBING → done) | Complex (PROBE_WAIT → PROBING → ANNOUNCE_WAIT → ANNOUNCING → ONGOING) |
| **Timer Mechanism** | Timeout callbacks | Periodic `acd_tmr()` calls |
| **Conflict Handling** | DHCP-specific callbacks | Generic ACD callbacks |
| **Probe Count** | 2 probes (via timeout) | 3 probes (PROBE_NUM) |
| **Announcement** | None | 2 announcements (ANNOUNCE_NUM) |

## Recommended Fixes for Static IP

### Fix #1: Automatic ACD Start on Static IP Assignment

```c
// In netif_do_set_ipaddr(), after setting IP address:
#if LWIP_ACD
  // If setting a new non-zero IP address (not removing)
  if (!ip4_addr_isany(ipaddr) && !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
    // Check if ACD is enabled and not already running for this address
    struct acd *acd;
    int acd_exists = 0;
    ACD_FOREACH(acd, netif->acd_list) {
      if (ip4_addr_eq(&acd->ipaddr, ipaddr)) {
        acd_exists = 1;
        break;
      }
    }
    
    // If no ACD module exists for this address, create and start one
    if (!acd_exists && LWIP_ACD_AUTO_START_STATIC) {
      // Application must provide callback via hook or config
      // Or use a default callback that logs conflicts
      // This requires application integration
    }
  }
#endif
```

**Alternative Approach:**
- Add configuration option: `LWIP_ACD_AUTO_START_STATIC`
- When enabled, automatically start ACD for static IP assignments
- Requires application to register a default conflict callback

### Fix #2: Defer IP Assignment Until ACD Completes

```c
// New function: netif_set_addr_with_acd()
err_t netif_set_addr_with_acd(struct netif *netif, 
                               const ip4_addr_t *ipaddr,
                               const ip4_addr_t *netmask,
                               const ip4_addr_t *gw,
                               acd_conflict_callback_t acd_callback)
{
  // 1. Start ACD BEFORE setting IP
  struct acd static_acd;
  acd_add(netif, &static_acd, acd_callback);
  acd_start(netif, &static_acd, *ipaddr);
  
  // 2. Wait for ACD_IP_OK callback
  // 3. Only then call netif_set_addr()
  
  // This requires async callback mechanism
}
```

**Challenge:** Requires asynchronous callback handling, which may not fit all application architectures.

### Fix #3: Add ACD State to netif Structure

```c
// Add to struct netif:
#if LWIP_ACD
  struct acd *static_ip_acd;  // ACD module for static IP
#endif

// In netif_set_addr():
#if LWIP_ACD
  if (!ip4_addr_isany(ipaddr)) {
    // Automatically manage ACD for static IP
    if (netif->static_ip_acd == NULL) {
      netif->static_ip_acd = mem_malloc(sizeof(struct acd));
      acd_add(netif, netif->static_ip_acd, default_static_ip_callback);
    }
    acd_start(netif, netif->static_ip_acd, *ipaddr);
  }
#endif
```

## Conclusion

The ACD implementation for static IP assignment has **critical flaws**:

1. **No automatic ACD start** - Applications must manually start ACD, which is error-prone
2. **IP assigned before conflict check** - Address is set and routing enabled before ACD confirms safety
3. **Race condition window** - Interface may route conflicting traffic before ACD starts
4. **Missing integration** - `netif_set_addr()` doesn't coordinate with ACD system
5. **State management** - ACD state not persisted across interface restarts

**Most Critical Issue:**
The fact that `netif_set_addr()` sets the IP address **immediately** without waiting for ACD conflict detection means that static IP addresses can be assigned and used even when conflicts exist. This defeats the purpose of ACD for static IP configurations.

**Recommendation:**
For production use with static IP and ACD enabled, applications **must**:
1. Call `acd_add()` and `acd_start()` immediately after `netif_set_addr()`
2. Wait for `ACD_IP_OK` callback before using the IP address
3. Handle `ACD_DECLINE` and `ACD_RESTART_CLIENT` callbacks appropriately
4. Consider the interface "not ready" until ACD completes successfully
5. Restart ACD after interface restarts

The current implementation places the burden of correct ACD integration entirely on the application developer, which is a significant design flaw.
