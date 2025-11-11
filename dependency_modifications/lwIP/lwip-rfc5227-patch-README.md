# LWIP RFC 5227 Compliance Patch

## Overview

This patch adds RFC 5227 compliant Address Conflict Detection (ACD) for static IP addresses to the ESP-IDF v5.5.1 LWIP stack. The patch implements deferred IP assignment, ensuring that static IP addresses are not assigned until ACD confirms they are safe to use.

## Patch Information

- **Target**: ESP-IDF v5.5.1 LWIP component
- **Base Path**: `components/lwip/`
- **Format**: Unified diff format
- **Files Modified**: 7 files
- **Files Created**: 1 new file

## Files Changed

### New Files
- `lwip/src/include/lwip/netif_pending_ip.h` - Pending IP configuration structure

### Modified Files
1. `lwip/src/include/lwip/opt.h` - Added `LWIP_ACD_RFC5227_COMPLIANT_STATIC` option
2. `lwip/src/include/lwip/netif.h` - Added `pending_ip_config` field and `netif_set_addr_with_acd()` function
3. `lwip/src/include/lwip/acd.h` - Added forward declaration for RFC 5227 callback
4. `lwip/src/core/ipv4/acd.c` - Implemented RFC 5227 compliant callback and conflict handling
5. `lwip/src/core/netif.c` - Implemented `netif_set_addr_with_acd()` function
6. `lwip/src/include/lwip/prot/acd.h` - Made ACD timing constants configurable
7. `port/include/lwipopts.h` - Added documentation for ACD timing overrides

## How to Apply

### Prerequisites
- ESP-IDF v5.5.1 installed
- Clean LWIP source (no previous modifications)

### Application Steps

1. **Navigate to ESP-IDF components directory:**
   ```bash
   cd $IDF_PATH/components/lwip
   ```

2. **Apply the patch:**
   ```bash
   patch -p1 < /path/to/lwip-rfc5227-patch.patch
   ```

   On Windows (PowerShell):
   ```powershell
   cd $env:IDF_PATH\components\lwip
   git apply --ignore-whitespace lwip-rfc5227-patch.patch
   ```

3. **Verify the patch applied correctly:**
   - Check that `netif_pending_ip.h` exists
   - Verify modified files contain the expected changes
   - Build your project to ensure no compilation errors

### Troubleshooting

If the patch fails to apply:

1. **Check ESP-IDF version:**
   ```bash
   cd $IDF_PATH
   git describe --tags
   ```
   Should show v5.5.1

2. **Verify clean source:**
   ```bash
   cd $IDF_PATH/components/lwip
   git status
   ```
   Should show no modifications (or only expected ones)

3. **Manual application:**
   If automatic patching fails, you can manually apply changes by following the diff in the patch file.

4. **Partial application:**
   If some hunks fail, you can use `patch -p1 --dry-run` to see what would be applied, then manually fix conflicts.

## Reverting the Patch

To revert the patch:

```bash
cd $IDF_PATH/components/lwip
patch -p1 -R < /path/to/lwip-rfc5227-patch.patch
```

Or restore from git:
```bash
cd $IDF_PATH/components/lwip
git checkout -- .
```

## Key Features

1. **RFC 5227 Compliance**: Static IP addresses are deferred until ACD confirms safety
2. **Configurable**: Can be enabled/disabled via `LWIP_ACD_RFC5227_COMPLIANT_STATIC`
3. **Backward Compatible**: Existing code using `netif_set_addr()` continues to work
4. **Protocol-Specific Timings**: ACD timing constants can be overridden for EtherNet/IP or other protocols
5. **Conflict Handling**: Properly removes IP addresses when conflicts are detected

## Configuration

The RFC 5227 compliant mode is enabled by default. To disable:

1. Define `LWIP_ACD_RFC5227_COMPLIANT_STATIC` to `0` in `lwipopts.h`, or
2. Modify `opt.h` to change the default value

## Usage

See `ReadmeACD.md` for detailed testing instructions and usage examples.

## Related Documentation

- `LWIP_MODIFICATIONS.md` - Complete documentation of all LWIP changes
- `ReadmeACD.md` - ACD testing guide
- `dependency_modifications/Analysis_of_ACD_Issue/RFC5227_COMPLIANCE_REQUIREMENTS.md` - RFC 5227 requirements analysis

## Notes

- This patch modifies the ESP-IDF LWIP component directly
- Consider maintaining a fork or using component overrides if you need to preserve ESP-IDF updates
- The patch is designed for ESP-IDF v5.5.1 and may need adjustments for other versions

