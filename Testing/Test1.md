# Test Setup 1 Results

![Test Setup 1](TestSetup1.png)

## Overview
- Screenshot shows the Allen-Bradley Micro850 ladder logic driving `ESP32p4_QT1[0]` through TON/TOF timer with the Saleae logic analyzer running behind it.
The Logic Analyzer was connected to the ESP32-P4 GPIO_PIN_33 which is mapped to Byte 0, Bit0 in OpENer.


## Observations
- Ladder timer block (TONOFF) updates the coil `ESP32p4_QT0[0]` with expected 5 ms ON/OFF periods.
- Saleae trace confirms 100 Hz output switching frequency (`≈ 9.99 ms` total period).
- Measured duty cycle 49.97% aligns with programmed 5 ms ton/toff.
- No watchdog trips or jitter artifacts observed during the capture window.

## Notes
- Micro850 Eth/IP module set to RPI of 5ms.
- Repeated test with varying TON/TOF values (e.g., 10 ms / 10 ms) to validate linear scaling.


