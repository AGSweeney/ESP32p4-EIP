# LWIP Version Comparison: 5.5.1 vs 6.0

## Overview
This document analyzes the differences between LWIP versions 5.5.1 and 6.0, focusing on changes that affect functionality, configuration, and API compatibility.

## Major Changes

### 1. Build System Changes (CMakeLists.txt)

#### Version 6.0 Changes:
- **Initialization**: Added explicit `set(srcs "")` at the beginning of the file for clearer initialization
- **Include Directories**: Removed `include/apps/sntp` from the include directories list
  - **Impact**: SNTP-specific headers are now accessed through the standard include path structure

#### Ping Application Changes:
- **Removed Files**: Version 6.0 removed two ping-related source files:
  - `apps/ping/esp_ping.c` - ESP-specific ping implementation wrapper
  - `apps/ping/ping.c` - Legacy ping implementation wrapper
- **Retained File**: Only `apps/ping/ping_sock.c` remains in version 6.0
  - **Impact**: The `esp_ping` API functions (`esp_ping_new_session`, `esp_ping_start`, `esp_ping_stop`, etc.) are still available and implemented in `ping_sock.c`. The consolidation removes duplicate implementations and simplifies maintenance. Applications using the `esp_ping.h` API should continue to work without changes, but the underlying implementation is now unified in the socket-based approach.

### 2. IPv6 Configuration Enhancements

#### IPv6 Forwarding Configuration:
- **Version 5.5.1**: `LWIP_IPV6_FORWARD` config option exists but lacks explicit dependency declaration
- **Version 6.0**: Added `depends on LWIP_IPV6` to `LWIP_IPV6_FORWARD` configuration
  - **Impact**: Ensures IPv6 forwarding can only be enabled when IPv6 is enabled, preventing configuration errors

#### IPv6 Router Advertisement DNS Server Option (RDNSS):
- **Status**: Both versions support RDNSS (RFC 6106)
- **Configuration**: `LWIP_IPV6_RDNSS_MAX_DNS_SERVERS` available in both versions
- **Location**: Configuration option moved slightly in Kconfig structure (lines 525 vs 538)

### 3. SNTP (Simple Network Time Protocol) Changes

#### Version 6.0 Enhancements:
- **Static Assertions**: Added compile-time checks to verify SNTP mode compatibility:
  ```c
  ESP_STATIC_ASSERT(SNTP_OPMODE_POLL == ESP_SNTP_OPMODE_POLL, ...);
  ESP_STATIC_ASSERT(SNTP_OPMODE_LISTENONLY == ESP_SNTP_OPMODE_LISTENONLY, ...);
  ```
  - **Impact**: Ensures LWIP SNTP modes match ESP-IDF enum values at compile time, preventing runtime mismatches

#### Copyright Update:
- **Version 5.5.1**: Copyright year 2015-2024
- **Version 6.0**: Copyright year 2015-2025

### 5. ACD (Address Conflict Detection) Changes

#### DHCP ACD Conflict Callback Behavior:
- **Version 5.5.1**: When a conflict is detected during DHCP address checking, only `ACD_DECLINE` callback is invoked
- **Version 6.0**: When a conflict is detected, **both** `ACD_DECLINE` and `ACD_RESTART_CLIENT` callbacks are invoked sequentially
  - **Impact**: Applications using ACD callbacks will receive an additional `ACD_RESTART_CLIENT` notification in version 6.0 when conflicts are detected during DHCP address checking. This provides more explicit notification that the DHCP client should restart its discovery process.

#### Core ACD Implementation:
- **Status**: The core ACD implementation in `lwip/src/core/ipv4/acd.c` appears unchanged between versions
- **MAC Address Comparison**: Both versions correctly implement MAC address comparison to avoid false positives from looped-back ARP packets (as documented in `acd-static-ip-issue.md`)

#### Configuration:
- Both versions support `LWIP_DHCP_DOES_ACD_CHECK` configuration option
- Both versions use the same port-specific implementation (`port/acd_dhcp_check.c`) for DHCP integration

### 6. IPv6 Core Implementation

#### Source Files:
Both versions contain identical IPv6 core source files:
- `dhcp6.c` - DHCPv6 client implementation
- `ethip6.c` - Ethernet IPv6 support
- `icmp6.c` - ICMPv6 protocol
- `inet6.c` - IPv6 internet functions
- `ip6.c` - IPv6 core implementation
- `ip6_addr.c` - IPv6 address handling
- `ip6_frag.c` - IPv6 fragmentation
- `mld6.c` - Multicast Listener Discovery for IPv6
- `nd6.c` - Neighbor Discovery Protocol for IPv6

**Note**: While the file structure is identical, internal implementations may have been updated. Review of individual source files would be needed for detailed code-level changes.

## Functional Impact Summary

### Breaking Changes:
1. **Ping Implementation Consolidation**: While the `esp_ping` API remains available, the implementation has been consolidated into `ping_sock.c`. Applications should continue to work, but internal behavior may differ slightly
2. **Include Paths**: SNTP include path changes may require updating include statements in applications (though standard includes should still work)
3. **ACD Callback Behavior**: Applications using ACD callbacks with DHCP will receive an additional `ACD_RESTART_CLIENT` callback in version 6.0 when conflicts are detected. Applications must handle this additional callback appropriately.

### Improvements:
1. **Configuration Safety**: Better dependency checking prevents invalid IPv6 forwarding configurations
2. **Compile-Time Safety**: SNTP mode assertions catch compatibility issues at build time
3. **Code Organization**: Cleaner build system initialization

### Migration Considerations:

#### For Applications Using Ping:
- **Before (5.5.1)**: Multiple ping implementations existed (`esp_ping.c`, `ping.c`, `ping_sock.c`)
- **After (6.0)**: Single unified implementation in `ping_sock.c` that provides the `esp_ping` API
- **Migration Path**: No migration needed for applications using `esp_ping.h` API - the API remains the same. The consolidation is transparent to application code. However, if applications directly included or linked against the removed files, they will need to update their build configuration.

#### For SNTP Applications:
- No API changes, but compile-time checks ensure compatibility
- Verify that SNTP mode enums match between LWIP and ESP-IDF versions

#### For Applications Using ACD:
- **Before (5.5.1)**: Conflict detection during DHCP checking only triggers `ACD_DECLINE` callback
- **After (6.0)**: Conflict detection triggers both `ACD_DECLINE` and `ACD_RESTART_CLIENT` callbacks
- **Migration Path**: Ensure ACD callback handlers can process both callback types. The `ACD_RESTART_CLIENT` callback indicates the DHCP client should restart its discovery process. Applications should handle this gracefully without causing duplicate restart operations.

## Configuration File Differences

### Kconfig Changes:
- **IPv6 Forwarding**: Added explicit dependency on `LWIP_IPV6` (line 532 in 6.0)
- **RDNSS**: Configuration option location shifted (lines 525 vs 538)
- **Overall Structure**: Minor organizational improvements in configuration grouping

### lwipopts.h Changes:
- Copyright year updated
- IPv6 forwarding configuration logic unchanged (both use `#ifdef CONFIG_LWIP_IPV6_FORWARD`)
- RDNSS configuration mapping unchanged

## Recommendations

1. **Before Upgrading**: 
   - Review build configurations that may reference removed ping source files (`esp_ping.c`, `ping.c`)
   - Verify that ping functionality continues to work as expected (API remains compatible)
   - **Review ACD callback handlers** - ensure they handle both `ACD_DECLINE` and `ACD_RESTART_CLIENT` callbacks
2. **Configuration Review**: Verify IPv6-related configurations are valid with new dependency checks
3. **Testing**: 
   - Test SNTP functionality to ensure mode compatibility
   - Test ping functionality to verify behavior matches expectations
   - **Test ACD conflict detection** - verify callback behavior matches expectations, especially for DHCP scenarios
4. **Build System**: Update any custom build scripts or CMakeLists that explicitly reference removed ping source files

## Conclusion

Version 6.0 represents a refinement release focusing on:
- Code simplification (ping API consolidation)
- Configuration safety improvements
- Compile-time error detection enhancements

The changes are generally non-breaking for most applications. The ping API consolidation is transparent to application code, though build configurations may need updates. The IPv6 implementation appears stable with only configuration improvements. Overall, version 6.0 focuses on code consolidation and improved configuration safety rather than major functional changes.

