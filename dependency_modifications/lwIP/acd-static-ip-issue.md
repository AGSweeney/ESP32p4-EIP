# ACD Static-IP Self-Conflict Fix

## Overview

When the ESP32 runs lwIP’s Address Conflict Detection (ACD) against a
statically configured IPv4 address, the stack can spuriously restart the
probe sequence. The wired Ethernet MAC reflects broadcast ARP probes back to
the host, and the generic `acd_arp_reply()` logic interprets those looped-back
frames as a conflict. As a result, the interface never reaches the `ACD_IP_OK`
state even though no other host is using the address.

## Root Cause

During the PROBE/PROBE_WAIT window the ACD implementation compares the sender
IP (`sipaddr`) against the candidate address and restarts the client as soon as
they match. Unlike the DHCP-specific hook, it does not verify that the sender
MAC differs from the interface MAC. Because the Ethernet driver delivers the
station’s own ARP requests to the host networking stack, this check misfires.
RFC 5227 section 2.2.1 requires the probing host to ignore its own packets, but
the current code path does not.

## Change Summary

- Update the PROBE/PROBE_WAIT clause in `lwip/src/core/ipv4/acd.c` to require
  both an IP match **and** a differing sender MAC before declaring a conflict.
- Keep the DHCP-port shim (`port/acd_dhcp_check.c`) unchanged; it already
  performs the correct MAC comparison.

```33:43:lwip/src/core/ipv4/acd.c
        if ((ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
             !eth_addr_eq(&netifaddr, &hdr->shwaddr)) ||
            (ip4_addr_isany_val(sipaddr) &&
             ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
             !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
          LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                      ("acd_arp_reply(): Probe Conflict detected\n"));
          acd_restart(netif, acd);
        }
```

## Validation

1. Configure the ESP32 with a static IPv4 address and enable ACD.
2. Build and flash with the updated `acd.c`.
3. Monitor `acd_conflict_callback()` or enable `ACD_DEBUG`.
4. Confirm that probing completes and `ACD_IP_OK` is emitted when no other
   device uses the address.

Automated coverage:

- Run the lwIP unit tests that exercise DHCP/ACD interactions (if available)
- Verify ACD state machine transitions in test scenarios

## Follow-Up

- Document the behavioral change in release notes.
- Consider backporting to maintenance branches that ship the ESP-specific
  ACD port.

