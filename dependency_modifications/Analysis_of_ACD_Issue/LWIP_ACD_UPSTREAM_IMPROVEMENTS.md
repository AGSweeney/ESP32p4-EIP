# ACD Improvements in Current lwIP Branch

## Summary

Based on analysis of the lwIP 5.5.1 codebase and changelog, there is **one significant improvement** related to ACD that affects resource usage and efficiency.

## Key Improvement: Conditional ACD Timer (Commit 353e8ff2)

### What Changed

The ACD timer (`acd_tmr()`) is now **conditionally enabled** based on DHCP ARP check configuration. This optimization was introduced in version 2.2.0.

### Implementation Details

**File**: `lwip/src/core/timeouts.c` (Line 96)
```c
#if LWIP_ACD && !DHCP_DOES_ARP_CHECK
  {ACD_TMR_INTERVAL, HANDLER(acd_tmr)},
#endif /* LWIP_ACD */
```

**File**: `lwip/src/include/lwip/opt.h` (Line 501)
```c
#define LWIP_NUM_SYS_TIMEOUT_INTERNAL   (LWIP_TCP + IP_REASSEMBLY + LWIP_ARP + 
                                         (ESP_LWIP_DHCP_FINE_TIMERS_ONDEMAND ? LWIP_DHCP : 2*LWIP_DHCP) + 
                                         (DHCP_DOES_ARP_CHECK ? 0 : LWIP_ACD) +  // ← KEY CHANGE
                                         ...)
```

### Why This Matters

1. **Memory Savings**: When `DHCP_DOES_ARP_CHECK` is enabled, the ACD timer is **not** added to the cyclic timers array, reducing `MEMP_NUM_SYS_TIMEOUT` by 1.

2. **CPU Savings**: The periodic `acd_tmr()` callback (runs every 100ms) is **not** scheduled when DHCP uses ARP check, saving CPU cycles.

3. **Correct Behavior**: DHCP with ARP check uses a port-specific implementation (`port/acd_dhcp_check.c`) that uses `sys_timeout()` callbacks instead of the periodic timer. The periodic timer is only needed for:
   - Static IP with ACD
   - AUTOIP with ACD
   - DHCP with ACD check (not ARP check)

### Impact

**Before the change:**
- ACD timer always enabled if `LWIP_ACD` was enabled
- Wasted resources when using DHCP with ARP check

**After the change:**
- ACD timer only enabled when actually needed
- More efficient resource usage
- No functional changes - just optimization

## Other ACD-Related Information

### ACD Implementation Status

1. **ACD was originally implemented** in 2018-10-04 by Jasper Verschueren (STABLE-2.2.0)

2. **Self-conflict detection fix** was already present in your 5.5.1 version:
   - MAC address comparison prevents detecting own ARP probes as conflicts
   - Fixed in `lwip/src/core/ipv4/acd.c` lines 410-414

3. **DHCP ACD integration** uses port-specific implementation:
   - `port/acd_dhcp_check.c` - Simplified ACD for DHCP (2 probes, ~1s)
   - Uses `sys_timeout()` callbacks instead of periodic timer
   - Faster than full RFC 5227 implementation

### What's NOT in Current Branch

Based on the changelog and code analysis, the following improvements are **NOT** present in the current lwIP branch:

1. **No RFC 5227 compliant static IP assignment** - IP is still assigned before ACD completes
2. **No automatic ACD start for static IP** - Still requires manual `acd_add()` and `acd_start()` calls
3. **No deferred IP assignment mechanism** - No API to defer IP assignment until ACD confirms safety

These are the improvements we implemented in your local 5.5.1 version with `LWIP_ACD_AUTO_START_STATIC`.

## Comparison: Current Branch vs Your Fixes

| Feature | Current lwIP Branch | Your 5.5.1 Fixes |
|---------|---------------------|------------------|
| **Conditional ACD Timer** | ✅ Yes (2.2.0) | ✅ Yes (already present) |
| **Self-Conflict Fix** | ✅ Yes | ✅ Yes |
| **Auto ACD Start for Static IP** | ❌ No | ✅ Yes (`LWIP_ACD_AUTO_START_STATIC`) |
| **RFC 5227 Compliant Static IP** | ❌ No | ❌ No (documented requirements) |
| **Deferred IP Assignment** | ❌ No | ❌ No (documented requirements) |

## Recommendations

1. **Keep Your Fixes**: The conditional timer optimization is already present in your version. Your `LWIP_ACD_AUTO_START_STATIC` fix provides functionality not available in the current upstream branch.

2. **Monitor Upstream**: The lwIP project is actively maintained. Watch for:
   - Any RFC 5227 compliance improvements
   - Static IP ACD enhancements
   - Additional ACD optimizations

3. **Consider Contributing**: Your `LWIP_ACD_AUTO_START_STATIC` implementation could be valuable to the upstream project. Consider submitting it as a patch.

## References

- **lwIP GitHub Repository**: https://github.com/lwip-tcpip/lwip
- **lwIP Official Site**: https://savannah.nongnu.org/projects/lwip/
- **Changelog Entry**: Commit 353e8ff2 - "timers: Conditionally enable ACD timer per DHCP ARP check (2.2.0)"

## Conclusion

The **conditional ACD timer** is the only significant ACD-related improvement in the current lwIP branch. It's an optimization that saves memory and CPU when using DHCP with ARP check, but doesn't address the RFC 5227 compliance issues or static IP ACD integration problems that your fixes address.

Your local improvements (`LWIP_ACD_AUTO_START_STATIC`) provide functionality that is **not** present in the current upstream branch, making them valuable additions to the codebase.

