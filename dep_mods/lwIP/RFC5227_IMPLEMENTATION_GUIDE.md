# RFC 5227 Compliance Implementation Guide for ESP-IDF LWIP

This document provides specific code changes needed to implement RFC 5227 compliant static IP assignment in ESP-IDF's LWIP stack.

**Target ESP-IDF Path**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\`

All file paths in this guide are relative to the ESP-IDF LWIP component directory above.

## Overview

The current implementation violates RFC 5227 because:
- IP is assigned **BEFORE** ACD completes (violates Section 2.1.1)
- IP is **NOT removed** when conflicts are detected (violates Section 2.4.1)

This guide implements deferred IP assignment: IP is assigned only after ACD confirms safety (ACD_IP_OK), matching DHCP behavior.

---

## Step 1: Add Configuration Option

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\include\lwip\opt.h`

**Location**: Add after other ACD-related options (around line 200-300, search for `LWIP_ACD`)

```c
/**
 * LWIP_ACD_RFC5227_COMPLIANT_STATIC==1: Enable RFC 5227 compliant static IP assignment.
 * When enabled, netif_set_addr() will defer IP assignment until ACD confirms safety.
 * IP address is NOT assigned until ACD_IP_OK callback is received.
 * If conflict is detected, IP is not assigned (or removed if already assigned).
 * 
 * This requires LWIP_ACD to be enabled.
 */
#ifndef LWIP_ACD_RFC5227_COMPLIANT_STATIC
#define LWIP_ACD_RFC5227_COMPLIANT_STATIC   0
#endif
#if !LWIP_ACD
#undef LWIP_ACD_RFC5227_COMPLIANT_STATIC
#define LWIP_ACD_RFC5227_COMPLIANT_STATIC   0
#endif
```

---

## Step 2: Add Pending IP Configuration Structure

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\include\lwip\netif_pending_ip.h` (NEW FILE)

**Create this new header file**:

```c
/**
 * @file
 * Pending IP configuration structure for RFC 5227 compliant static IP assignment
 */

#ifndef LWIP_HDR_NETIF_PENDING_IP_H
#define LWIP_HDR_NETIF_PENDING_IP_H

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC

#include "lwip/ip4_addr.h"
#include "lwip/acd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct netif_pending_ip_config {
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  struct acd acd;
  acd_conflict_callback_t user_callback;
  u8_t pending;  /* 1 if waiting for ACD, 0 if assigned */
};

#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV4 && LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC */

#endif /* LWIP_HDR_NETIF_PENDING_IP_H */
```

---

## Step 3: Modify netif.h Structure

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\include\lwip\netif.h`

**Location**: Inside `struct netif` definition (around line 200-400, search for `struct netif`)

**Add after other ACD-related fields**:

```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
  struct netif_pending_ip_config *pending_ip_config;
#endif
```

**Also add include at top of file** (after other includes):

```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
#include "lwip/netif_pending_ip.h"
#endif
```

---

## Step 4: Add RFC 5227 Compliant Callback to acd.c

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\ipv4\acd.c`

**Location**: Add after `acd_put_in_passive_mode()` function (around line 590)

**Add this new function**:

```c
#if LWIP_ACD_RFC5227_COMPLIANT_STATIC
/**
 * RFC 5227 compliant callback for static IP assignment
 * Assigns IP only after ACD confirms safety (ACD_IP_OK)
 * Removes IP if conflict detected
 */
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
      
      pending->pending = 0;  /* Mark as assigned */
      
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
#endif /* LWIP_ACD_RFC5227_COMPLIANT_STATIC */
```

---

## Step 5: Modify Conflict Handling in acd.c

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\ipv4\acd.c`

**Location**: Modify `acd_handle_arp_conflict()` function (around line 517-560)

**Add RFC 5227 compliant conflict handling**:

```c
static void
acd_handle_arp_conflict(struct netif *netif, struct acd *acd)
{
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
#endif /* LWIP_ACD_RFC5227_COMPLIANT_STATIC */

  /* RFC5227, 2.4 "Ongoing Address Conflict Detection and Address Defense"
     ... existing code continues here ... */
  
  if (acd->state == ACD_STATE_PASSIVE_ONGOING) {
    /* Immediately back off on a conflict. */
    LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
      ("acd_handle_arp_conflict(): conflict when we are in passive mode -> back off\n"));
    acd_stop(acd);
    acd->acd_conflict_callback(netif, ACD_DECLINE);
  }
  else {
    if (acd->lastconflict > 0) {
      /* retreat, there was a conflicting ARP in the last DEFEND_INTERVAL seconds */
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
        ("acd_handle_arp_conflict(): conflict within DEFEND_INTERVAL -> retreating\n"));

      /* Active TCP sessions are aborted when removing the ip address but a bad
       * connection was inevitable anyway with conflicting hosts */
       acd_restart(netif, acd);
    } else {
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
          ("acd_handle_arp_conflict(): we are defending, send ARP Announce\n"));
      etharp_acd_announce(netif, &acd->ipaddr);
      acd->lastconflict = DEFEND_INTERVAL * ACD_TICKS_PER_SECOND;
    }
  }
}
```

**Note**: You'll need to add `#include "lwip/netif_pending_ip.h"` at the top of `acd.c` if not already present.

---

## Step 6: Implement netif_set_addr_with_acd() Function

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\netif.c`

**Location**: Add after `netif_set_addr()` function (search for `netif_set_addr`, around line 500-600)

**Add this new function**:

```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
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
#endif /* LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC */
```

**Add function declaration to**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\include\lwip\netif.h`

**Location**: Add in the function declarations section (around line 600-800)

```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
err_t netif_set_addr_with_acd(struct netif *netif,
                               const ip4_addr_t *ipaddr,
                               const ip4_addr_t *netmask,
                               const ip4_addr_t *gw,
                               acd_conflict_callback_t callback);
#endif
```

**Add includes at top of netif.c** (if not already present):

```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
#include "lwip/netif_pending_ip.h"
#include "lwip/acd.h"
#endif
```

---

## Step 7: (Optional) Modify netif_set_addr() for Automatic RFC 5227 Mode

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\netif.c`

**Location**: Modify `netif_set_addr()` function (around line 500-600)

**Add RFC 5227 mode check at the beginning**:

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
  netif_do_set_ipaddr(netif, ipaddr, NULL);
  netif_do_set_netmask(netif, netmask, NULL);
  netif_do_set_gw(netif, gw, NULL);
}
```

**Note**: This makes `netif_set_addr()` automatically use RFC 5227 mode when enabled. If you prefer explicit API only, skip this step.

---

## Step 8: Clean Up Pending Config on netif Removal

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\netif.c`

**Location**: In `netif_remove()` function (search for it, around line 200-400)

**Add cleanup code**:

```c
#if LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC
  /* Clean up pending IP configuration if present */
  if (netif->pending_ip_config != NULL) {
    acd_remove(netif, &netif->pending_ip_config->acd);
    mem_free(netif->pending_ip_config);
    netif->pending_ip_config = NULL;
  }
#endif
```

---

## Step 9: Add Forward Declaration

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\ipv4\acd.c`

**Location**: At top of file, add forward declaration (around line 165, with other static function declarations)

```c
#if LWIP_ACD_RFC5227_COMPLIANT_STATIC
static void acd_static_ip_rfc5227_callback(struct netif *netif, acd_callback_enum_t state);
#endif
```

---

## Step 10: Update ESP-IDF Configuration

**File**: `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\port\include\lwipopts.h` (ESP-IDF specific)

**Or via menuconfig**: Navigate to `Component config > LWIP` and add the option there.

**Add configuration option** (or set via menuconfig):

```c
#define LWIP_ACD_RFC5227_COMPLIANT_STATIC   1
```

**Or add to ESP-IDF menuconfig**:
- Navigate to: `Component config > LWIP > Enable RFC 5227 compliant static IP assignment`
- Add option in `components/lwip/Kconfig`

---

## Testing Checklist

1. **Normal Case**:
   - Call `netif_set_addr_with_acd()` with non-conflicting IP
   - Verify IP assigned only after ACD_IP_OK callback
   - Verify IP is usable after callback

2. **Conflict Detection**:
   - Set conflicting IP on another device
   - Call `netif_set_addr_with_acd()` with conflicting IP
   - Verify ACD_DECLINE callback received
   - Verify IP NOT assigned

3. **Ongoing Conflict**:
   - Assign IP successfully
   - Introduce conflict after assignment
   - Verify IP removed and callback received

4. **Backward Compatibility**:
   - Verify existing code using `netif_set_addr()` still works when RFC 5227 mode disabled
   - Verify DHCP behavior unchanged

---

## Migration Notes

- **Phase 1**: Implement `netif_set_addr_with_acd()` API (new function, doesn't break existing code)
- **Phase 2**: Add configuration option for automatic RFC 5227 mode
- **Phase 3**: Update application code to use new API
- **Phase 4**: Consider making RFC 5227 mode default in future versions

---

## Files Modified Summary

1. ✅ `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\include\lwip\opt.h` - Configuration option
2. ✅ `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\include\lwip\netif_pending_ip.h` - NEW FILE - Pending config structure
3. ✅ `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\include\lwip\netif.h` - Add pending_ip_config field and function declaration
4. ✅ `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\ipv4\acd.c` - RFC 5227 callback and conflict handling
5. ✅ `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\lwip\src\core\netif.c` - New API function and optional netif_set_addr() modification
6. ✅ `C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\port\include\lwipopts.h` - ESP-IDF configuration (or Kconfig)

---

## References

- See `dep_mods/AI_Analysis_Of_Issue/RFC5227_COMPLIANCE_REQUIREMENTS.md` for detailed requirements
- RFC 5227: https://tools.ietf.org/html/rfc5227
- Current ACD implementation: `dep_mods/lwIP/acd.c`

---

## Quick Reference: Full File Paths

All modifications are made to files in:
```
C:\Users\agswe\esp\v5.5.1\esp-idf\components\lwip\
```

Relative paths from that directory:
- `lwip\src\include\lwip\opt.h`
- `lwip\src\include\lwip\netif_pending_ip.h` (NEW)
- `lwip\src\include\lwip\netif.h`
- `lwip\src\core\ipv4\acd.c`
- `lwip\src\core\netif.c`
- `port\include\lwipopts.h`

