# RFC 5227 Compliance for Static IP Assignment

## RFC 5227 Requirements

RFC 5227 (IPv4 Address Conflict Detection) specifies:

1. **Section 2.1 - Probing**: Before using an IPv4 address, a host MUST check if it's already in use by sending ARP probes.

2. **Section 2.1.1**: "A host MUST NOT use an IPv4 address if a conflict is detected during the probing phase."

3. **Section 2.4 - Ongoing Detection**: After successfully claiming an address, a host MUST continue to monitor for conflicts and defend its address.

4. **Section 2.4.1**: "If a conflict is detected, the host MUST immediately stop using the address."

## Current State (Non-Compliant)

### Problems:

1. **IP Assigned Before ACD Completes** ❌
   - `netif_set_addr()` assigns IP immediately (line 505 in `netif.c`)
   - ACD starts AFTER IP is already assigned
   - Violates RFC 5227 Section 2.1.1: "MUST NOT use an IPv4 address if a conflict is detected"

2. **IP Not Removed on Conflict** ❌
   - Default callback only logs conflicts
   - IP remains assigned even when conflict detected
   - Violates RFC 5227 Section 2.4.1: "MUST immediately stop using the address"

3. **No Deferred Assignment Mechanism** ❌
   - No API to defer IP assignment until ACD confirms safety
   - Cannot match DHCP behavior (assign after ACD_IP_OK)

## Required Changes for Full RFC 5227 Compliance

### 1. New API: Deferred IP Assignment

Create a new function that defers IP assignment until ACD confirms safety:

```c
/**
 * Set static IP address with RFC 5227 compliant conflict detection
 * 
 * IP address is NOT assigned until ACD confirms it's safe (ACD_IP_OK callback).
 * If conflict is detected, IP is not assigned and callback is notified.
 * 
 * @param netif Network interface
 * @param ipaddr IP address to assign (after ACD confirms)
 * @param netmask Netmask
 * @param gw Gateway
 * @param callback Callback for ACD events (required)
 * @return ERR_OK if ACD started successfully, ERR_ARG if invalid params
 */
err_t netif_set_addr_with_acd(struct netif *netif,
                               const ip4_addr_t *ipaddr,
                               const ip4_addr_t *netmask,
                               const ip4_addr_t *gw,
                               acd_conflict_callback_t callback);
```

### 2. Pending IP Configuration Storage

Add structure to store pending IP configuration:

```c
struct netif_pending_ip_config {
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  struct acd acd;
  acd_conflict_callback_t user_callback;
  u8_t pending;  // 1 if waiting for ACD, 0 if assigned
};
```

Add to `struct netif`:
```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
  struct netif_pending_ip_config *pending_ip_config;
#endif
```

### 3. Modified ACD Callback for Static IP

Create RFC 5227 compliant callback:

```c
static void
acd_static_ip_rfc5227_callback(struct netif *netif, acd_callback_enum_t state)
{
  struct netif_pending_ip_config *pending = netif->pending_ip_config;
  
  if (pending == NULL) {
    return;
  }
  
  switch (state) {
    case ACD_IP_OK:
      /* RFC 5227 Section 2.1: Address confirmed safe, now assign it */
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                  ("acd_static_ip_rfc5227_callback(): Address confirmed safe, assigning IP\n"));
      
      /* Assign IP address */
      netif_do_set_ipaddr(netif, &pending->ipaddr, NULL);
      netif_do_set_netmask(netif, &pending->netmask, NULL);
      netif_do_set_gw(netif, &pending->gw, NULL);
      
      pending->pending = 0;  // Mark as assigned
      
      /* Notify user callback */
      if (pending->user_callback) {
        pending->user_callback(netif, ACD_IP_OK);
      }
      break;
      
    case ACD_DECLINE:
    case ACD_RESTART_CLIENT:
      /* RFC 5227 Section 2.4.1: Conflict detected, MUST NOT use address */
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                  ("acd_static_ip_rfc5227_callback(): Conflict detected, NOT assigning IP\n"));
      
      /* Clean up pending configuration */
      acd_remove(netif, &pending->acd);
      pending->pending = 0;
      
      /* Notify user callback */
      if (pending->user_callback) {
        pending->user_callback(netif, state);
      }
      
      /* Free pending config */
      mem_free(pending);
      netif->pending_ip_config = NULL;
      break;
      
    default:
      break;
  }
}
```

### 4. Modified netif_set_addr() with RFC 5227 Mode

Add optional RFC 5227 compliant mode:

```c
void
netif_set_addr(struct netif *netif, const ip4_addr_t *ipaddr, 
                const ip4_addr_t *netmask, const ip4_addr_t *gw)
{
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
  /* If RFC 5227 compliant mode enabled and ACD enabled, defer assignment */
  if (LWIP_ACD_RFC5227_COMPLIANT_STATIC && 
      !ip4_addr_isany(ipaddr) && 
      !ip_addr_islinklocal(ip_2_ip4(ipaddr))) {
    
    /* Check if we should defer assignment */
    if (netif->pending_ip_config == NULL) {
      /* Start deferred assignment */
      netif_set_addr_with_acd(netif, ipaddr, netmask, gw, NULL);
      return;
    }
  }
#endif
  
  /* Original immediate assignment code */
  /* ... existing code ... */
}
```

### 5. Implementation of netif_set_addr_with_acd()

```c
err_t
netif_set_addr_with_acd(struct netif *netif,
                        const ip4_addr_t *ipaddr,
                        const ip4_addr_t *netmask,
                        const ip4_addr_t *gw,
                        acd_conflict_callback_t callback)
{
  struct netif_pending_ip_config *pending;
  
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ERROR("netif != NULL", netif != NULL, return ERR_ARG);
  LWIP_ERROR("ipaddr != NULL", ipaddr != NULL, return ERR_ARG);
  LWIP_ERROR("netmask != NULL", netmask != NULL, return ERR_ARG);
  LWIP_ERROR("gw != NULL", gw != NULL, return ERR_ARG);
  
  /* Don't allow if already pending */
  if (netif->pending_ip_config != NULL) {
    LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE, 
                ("netif_set_addr_with_acd(): Already have pending IP config\n"));
    return ERR_INPROGRESS;
  }
  
  /* Allocate pending configuration */
  pending = (struct netif_pending_ip_config *)mem_malloc(sizeof(struct netif_pending_ip_config));
  if (pending == NULL) {
    return ERR_MEM;
  }
  
  /* Store configuration */
  ip4_addr_copy(pending->ipaddr, *ipaddr);
  ip4_addr_copy(pending->netmask, *netmask);
  ip4_addr_copy(pending->gw, *gw);
  pending->user_callback = callback;
  pending->pending = 1;
  
  netif->pending_ip_config = pending;
  
  /* Start ACD BEFORE assigning IP (RFC 5227 compliant) */
  if (acd_add(netif, &pending->acd, acd_static_ip_rfc5227_callback) == ERR_OK) {
    if (acd_start(netif, &pending->acd, *ipaddr) == ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                  ("netif_set_addr_with_acd(): ACD started, IP assignment deferred\n"));
      return ERR_OK;
    }
  }
  
  /* Failed to start ACD, clean up */
  mem_free(pending);
  netif->pending_ip_config = NULL;
  return ERR_VAL;
}
```

### 6. Ongoing Conflict Defense

Modify `acd_handle_arp_conflict()` to remove IP if conflict detected after assignment:

```c
static void
acd_handle_arp_conflict(struct netif *netif, struct acd *acd)
{
  /* ... existing code ... */
  
#if LWIP_ACD_RFC5227_COMPLIANT_STATIC
  /* RFC 5227 Section 2.4.1: If conflict detected, MUST stop using address */
  if (netif->pending_ip_config != NULL && 
      ip4_addr_eq(&acd->ipaddr, &netif->pending_ip_config->ipaddr)) {
    /* This is a static IP with RFC 5227 compliance */
    LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                ("acd_handle_arp_conflict(): RFC 5227 - Removing conflicting static IP\n"));
    
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
  
  /* ... existing code for other cases ... */
}
```

### 7. Configuration Option

Add to `lwip/src/include/lwip/opt.h`:

```c
/**
 * LWIP_ACD_RFC5227_COMPLIANT_STATIC==1: Enable RFC 5227 compliant static IP assignment.
 * When enabled, netif_set_addr() will defer IP assignment until ACD confirms safety.
 * IP address is NOT assigned until ACD_IP_OK callback is received.
 * If conflict is detected, IP is not assigned (or removed if already assigned).
 * 
 * This requires LWIP_ACD to be enabled.
 */
#if !defined LWIP_ACD_RFC5227_COMPLIANT_STATIC || defined __DOXYGEN__
#define LWIP_ACD_RFC5227_COMPLIANT_STATIC   0
#endif
#if !LWIP_ACD
#undef LWIP_ACD_RFC5227_COMPLIANT_STATIC
#define LWIP_ACD_RFC5227_COMPLIANT_STATIC   0
#endif
```

## Implementation Steps

### Step 1: Add Pending IP Configuration Structure

**File**: `lwip/src/include/lwip/netif.h`

Add to `struct netif`:
```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
  struct netif_pending_ip_config *pending_ip_config;
#endif
```

Create new header file `lwip/src/include/lwip/netif_pending_ip.h`:
```c
struct netif_pending_ip_config {
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  struct acd acd;
  acd_conflict_callback_t user_callback;
  u8_t pending;
};
```

### Step 2: Implement RFC 5227 Compliant Callback

**File**: `lwip/src/core/ipv4/acd.c`

Add the `acd_static_ip_rfc5227_callback()` function as shown above.

### Step 3: Implement netif_set_addr_with_acd()

**File**: `lwip/src/core/netif.c`

Add the new API function as shown above.

### Step 4: Modify Conflict Handling

**File**: `lwip/src/core/ipv4/acd.c`

Modify `acd_handle_arp_conflict()` to remove IP on conflict for RFC 5227 compliant static IPs.

### Step 5: Update netif_set_addr() (Optional)

**File**: `lwip/src/core/netif.c`

Optionally modify `netif_set_addr()` to automatically use RFC 5227 mode when enabled.

## Usage

### Option 1: Explicit RFC 5227 API

```c
void my_acd_callback(struct netif *netif, acd_callback_enum_t state)
{
    switch (state) {
        case ACD_IP_OK:
            printf("IP address confirmed safe and assigned\n");
            break;
        case ACD_DECLINE:
        case ACD_RESTART_CLIENT:
            printf("Conflict detected, IP not assigned\n");
            // Handle conflict (e.g., try different IP, alert user)
            break;
    }
}

ip4_addr_t ip, netmask, gw;
IP4_ADDR(&ip, 192, 168, 1, 100);
IP4_ADDR(&netmask, 255, 255, 255, 0);
IP4_ADDR(&gw, 192, 168, 1, 1);

// RFC 5227 compliant: IP assigned only after ACD confirms safety
err_t err = netif_set_addr_with_acd(netif, &ip, &netmask, &gw, my_acd_callback);
if (err == ERR_OK) {
    // IP will be assigned when ACD_IP_OK callback is received
    // Do NOT use IP address until callback confirms
}
```

### Option 2: Automatic RFC 5227 Mode

```c
// Enable in lwipopts.h:
#define LWIP_ACD_RFC5227_COMPLIANT_STATIC   1

// Then use normal API:
netif_set_addr(netif, &ip, &netmask, &gw);
// Automatically defers assignment until ACD confirms
```

## Benefits

1. **RFC 5227 Compliant**: IP assigned only after ACD confirms safety
2. **No Race Condition**: No window where conflicting IP is routed
3. **Proper Conflict Handling**: IP removed if conflict detected
4. **Matches DHCP Behavior**: Same timing as DHCP (assign after ACD_IP_OK)
5. **Backward Compatible**: Optional feature, defaults to disabled

## Testing

1. **Test Normal Case**:
   - Call `netif_set_addr_with_acd()` with non-conflicting IP
   - Verify IP assigned only after ACD_IP_OK callback
   - Verify IP is usable after callback

2. **Test Conflict Detection**:
   - Set conflicting IP on another device
   - Call `netif_set_addr_with_acd()` with conflicting IP
   - Verify ACD_DECLINE callback received
   - Verify IP NOT assigned

3. **Test Ongoing Conflict**:
   - Assign IP successfully
   - Introduce conflict after assignment
   - Verify IP removed and callback received

## Migration Path

1. **Phase 1**: Implement `netif_set_addr_with_acd()` API (new function, doesn't break existing code)
2. **Phase 2**: Add configuration option for automatic RFC 5227 mode
3. **Phase 3**: Update documentation and examples
4. **Phase 4**: Consider making RFC 5227 mode default in future versions

## Summary

To achieve **full RFC 5227 compliance** for static IP assignment, we need:

1. ✅ **Defer IP assignment** until ACD confirms safety (ACD_IP_OK)
2. ✅ **Remove IP** if conflict detected (ACD_DECLINE)
3. ✅ **Remove IP** if ongoing conflict detected (Section 2.4.1)
4. ✅ **New API** to support deferred assignment
5. ✅ **Pending configuration storage** to hold IP until ACD completes
6. ✅ **RFC 5227 compliant callback** that assigns/removes IP based on ACD results

The current implementation violates RFC 5227 because:
- IP is assigned BEFORE ACD completes
- IP is not removed when conflicts are detected

The proposed solution makes static IP assignment RFC 5227 compliant by matching DHCP's behavior: assign IP only after ACD confirms safety.

