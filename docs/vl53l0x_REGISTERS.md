# VL53L0X Register Map Cheatsheet

This driver uses a focused subset of the VL53L0X register map.  
The table below summarises each `#define VL53L0X_REG_*` symbol in `vl53l0x.c`, the register address, and the purpose / typical options you might care about when integrating the sensor into other projects.

| Macro | Address | Description & Usage |
| --- | --- | --- |
| `VL53L0X_REG_SYSRANGE_START` | `0x00` | Control register used to start ranging. Writing `0x01` kicks off a single shot or continuous measurement sequence depending on other settings. |
| `VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG` | `0x01` | Enables/disables parts of the ranging sequence (TCC, DSS, MSRC, pre-range, final-range). Each bit gates a stage; the driver programs this when switching between calibration steps. |
| `VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD` | `0x04` | When the device is in continuous mode, this sets the delay (in milliseconds) between consecutive range measurements. |
| `VL53L0X_REG_SYSTEM_RANGE_CONFIG` | `0x09` | Miscellaneous system-level range configuration; ST’s reference code leaves this at default. |
| `VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO` | `0x0A` | Configures which event drives the GPIO interrupt line. The driver selects “new sample ready” so we can use the interrupt status register. |
| `VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR` | `0x0B` | Write `0x01` after each reading to clear the interrupt flags. |
| `VL53L0X_REG_RESULT_INTERRUPT_STATUS` | `0x13` | Primary status register. Bits `[2:0]` report measurement completion codes (0 = busy, 1/2/3/4/… = various “range ready” states). |
| `VL53L0X_REG_RESULT_RANGE_STATUS` | `0x14` | Holds detailed range results: bits `[15:0]` contain all result fields, while the driver reads the main range-status code (`(value >> 3) & 0x1F`). |
| `VL53L0X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS` | `0x20` | Crosstalk compensation limit; ST tunes this during calibration. Left untouched unless you are re-writing the tuning profile. |
| `VL53L0X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM` | `0x28` | Factory-programmed part-to-part offset. You can read it for diagnostics; writing a new value lets you apply your own calibration offset. |
| `VL53L0X_REG_VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND` | `0x30` | Timeout setting involved in VHV (Very High Voltage) calibration loops. Required during sensor bring-up. |
| `VL53L0X_REG_MSRC_CONFIG_CONTROL` | `0x60` | Enables or disables the MSRC (Minimum Signal Rate Check). The driver forces bitfields so MSRC uses the same signal rate as the final range stage. |
| `VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT` | `0x44` | Minimum return signal rate (in MCPS) required for a valid final range measurement. Lower the value to accept weaker targets, higher it to reject noisy readings. |
| `VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP` | `0x46` | Macro-period timeout for the MSRC stage. |
| `VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD` | `0x50` | Sets the VCSEL pulse period for the pre-range stage (in PCLKs). Typical values: `0x18`, `0x1C`. |
| `VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI`<br/>`VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO` | `0x51` / `0x52` | Combined to form the pre-range timeout (encoded with ST’s special format). |
| `VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD` | `0x70` | VCSEL period for the final range stage. Together with the timing budget this controls maximum distance vs. speed. |
| `VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI`<br/>`VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO` | `0x71` / `0x72` | Encoded timeout for the final range stage. The driver rewrites these when you change the timing budget. |
| `VL53L0X_REG_RESULT_RANGE_STATUS + 10` | `0x1E` | (Not a named macro, but used in the driver.) Reading two bytes from here yields the measured distance in millimetres (big-endian). |
| `VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV` | `0x89` | Controls voltage level on I²C pads. Bit 0 selects between 1.8 V and external supply (>2.8 V). The driver sets this when initialising. |
| `VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH` | `0x84` | Chooses the polarity of the interrupt pin. Clearing bit 4 makes the interrupt active-low, matching ST’s reference design. |
| `VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD` | `0x4E` | Number of reference SPADs to enable dynamically. ST’s tuning sets this to `0x2C`. |
| `VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET` | `0x4F` | Start offset when enabling reference SPADs. |
| `VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0` | `0xB0` | Start of a 6-byte bitmap that enables individual SPADs. The driver writes a tuned mask after sensor init. |
| `VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT` | `0xB6` | Selects first SPAD to enable (for reference). Works with the dynamic SPAD registers above. |
| `VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS` | `0x8A` | Current 7-bit I²C address (`0x29` by default). Writing a new value lets you readdress the device. |
| `VL53L0X_REG_IDENTIFICATION_MODEL_ID` | `0xC0` | Read-only ID register. Expected value is `0xEE`. If you read something else (e.g., `0xC0`), the sensor has not completed boot. |

## Practical tips

- **Timing budgets & VCSEL periods:** Adjust the pre-range/final-range VCSEL period (`0x50` / `0x70`) and the associated encoded timeouts (`0x51`/`0x52`, `0x71`/`0x72`) if you want longer range or faster updates. The helper in `vl53l0x.c` does this for you based on a single microsecond budget.
- **Interrupt handling:** Use `VL53L0X_REG_RESULT_INTERRUPT_STATUS` to poll for data-ready. Clear it via `VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR`.
- **Address changes:** Write a new 7-bit address to `VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS` immediately after initialising if you have multiple sensors on the same bus.
- **Detecting boot completion:** Poll `VL53L0X_REG_IDENTIFICATION_MODEL_ID` until you read `0xEE`. Reading `0xC0` indicates the device is still in reset.

These notes cover the registers referenced directly in the driver. For a full register map and bit-field breakdown, consult ST’s “VL53L0X API user manual” (UM2039) and datasheet (DS10806).

