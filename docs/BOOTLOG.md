# ESP32-P4 Boot Log (ANSI Styled)

> Captured via `idf.py monitor` on the Waveshare ESP32-P4-NANO with PoE module.

```ansi
\u001b[36mPS C:\Users\Adam\ESP32-P4-OpENerEIP> $env:IDF_PATH = 'C:\Users\Adam\esp\v5.5.1\esp-idf';\u001b[0m
\u001b[36mPS C:\Users\Adam\ESP32-P4-OpENerEIP>  & 'C:\Users\Adam\.espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe' 'C:\Users\Adam\esp\v5.5.1\esp-idf\tools\idf_monitor.py' -p COM4 -b 115200 --toolchain-prefix riscv32-esp-elf- --make '''C:\Users\Adam\.espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe'' ''C:\Users\Adam\esp\v5.5.1\esp-idf\tools\idf.py''' --target esp32p4 'c:\Users\Adam\ESP32-P4-OpENerEIP\build\ESP32-P4-OpENerEIP.elf'\u001b[0m
\u001b[33m--- Warning: GDB cannot open serial ports accessed as COMx\u001b[0m
\u001b[33m--- Using \\.\COM4 instead...\u001b[0m
--- esp-idf-monitor 1.8.0 on \\.\COM4 115200
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
ESP-ROM:esp32p4-eco2-20240710
Build:Jul 10 2024
rst:0x1 (POWERON),boot:0x20f (SPI_FAST_FLASH_BOOT)
SPI mode:DIO, clock div:1
load:0x4ff33ce0,len:0x164c
load:0x4ff29ed0,len:0xdfc
load:0x4ff2cbd0,len:0x336c
entry 0x4ff29eda
\u001b[32mI (25) boot: ESP-IDF v5.5.1-dirty 2nd stage bootloader\u001b[0m
\u001b[32mI (26) boot: compile time Nov  8 2025 17:12:45\u001b[0m
\u001b[32mI (26) boot: Multicore bootloader\u001b[0m
\u001b[32mI (28) boot: chip revision: v1.0\u001b[0m
\u001b[32mI (29) boot: efuse block revision: v0.3\u001b[0m
\u001b[32mI (33) boot.esp32p4: SPI Speed      : 80MHz\u001b[0m
\u001b[32mI (37) boot.esp32p4: SPI Mode       : DIO\u001b[0m
\u001b[32mI (40) boot.esp32p4: SPI Flash Size : 16MB\u001b[0m
\u001b[32mI (44) boot: Enabling RNG early entropy source...\u001b[0m
\u001b[32mI (49) boot: Partition Table:\u001b[0m
\u001b[32mI (51) boot: ## Label            Usage          Type ST Offset   Length\u001b[0m
\u001b[32mI (58) boot:  0 nvs              WiFi data        01 02 00009000 00006000\u001b[0m
\u001b[32mI (64) boot:  1 phy_init         RF data          01 01 0000f000 00001000\u001b[0m
\u001b[32mI (71) boot:  2 factory          factory app      00 00 00010000 00100000\u001b[0m
\u001b[32mI (78) boot: End of partition table\u001b[0m
\u001b[32mI (81) esp_image: segment 0: paddr=00010020 vaddr=40060020 size=173e0h ( 95200) map\u001b[0m
\u001b[32mI (105) esp_image: segment 1: paddr=00027408 vaddr=30100000 size=00088h (   136) load\u001b[0m
\u001b[32mI (107) esp_image: segment 2: paddr=00027498 vaddr=4ff00000 size=08b80h ( 35712) load\u001b[0m
\u001b[32mI (117) esp_image: segment 3: paddr=00030020 vaddr=40000020 size=57698h (358040) map\u001b[0m
\u001b[32mI (178) esp_image: segment 4: paddr=000876c0 vaddr=4ff08b80 size=07ccch ( 31948) load\u001b[0m
\u001b[32mI (186) esp_image: segment 5: paddr=0008f394 vaddr=4ff10880 size=0241ch (  9244) load\u001b[0m
\u001b[32mI (193) boot: Loaded app from partition at offset 0x10000\u001b[0m
\u001b[32mI (193) boot: Disabling RNG early entropy source...\u001b[0m
\u001b[32mI (206) cpu_start: Multicore app\u001b[0m
\u001b[32mI (215) cpu_start: Pro cpu start user code\u001b[0m
\u001b[32mI (216) cpu_start: cpu freq: 360000000 Hz\u001b[0m
\u001b[32mI (216) app_init: Application information:\u001b[0m
\u001b[32mI (216) app_init: Project name:     ESP32-P4-OpENerEIP\u001b[0m
\u001b[32mI (220) app_init: App version:      1\u001b[0m
\u001b[32mI (224) app_init: Compile time:     Nov  8 2025 17:12:29\u001b[0m
\u001b[32mI (229) app_init: ELF file SHA256:  e41be1073...\u001b[0m
\u001b[32mI (233) app_init: ESP-IDF:          v5.5.1-dirty\u001b[0m
\u001b[32mI (237) efuse_init: Min chip rev:     v0.1\u001b[0m
\u001b[32mI (241) efuse_init: Max chip rev:     v1.99\u001b[0m
\u001b[32mI (245) efuse_init: Chip rev:         v1.0\u001b[0m
\u001b[32mI (249) heap_init: Initializing. RAM available for dynamic allocation:\u001b[0m
\u001b[32mI (255) heap_init: At 4FF18300 len 00022CC0 (139 KiB): RAM\u001b[0m
\u001b[32mI (261) heap_init: At 4FF3AFC0 len 00004BF0 (18 KiB): RAM\u001b[0m
\u001b[32mI (266) heap_init: At 4FF40000 len 00060000 (384 KiB): RAM\u001b[0m
\u001b[32mI (271) heap_init: At 50108080 len 00007F80 (31 KiB): RTCRAM\u001b[0m
\u001b[32mI (276) heap_init: At 30100088 len 00001F78 (7 KiB): TCM\u001b[0m
\u001b[32mI (282) spi_flash: detected chip: gd\u001b[0m
\u001b[32mI (285) spi_flash: flash io: dio\u001b[0m
\u001b[32mI (288) main_task: Started on CPU0\u001b[0m
\u001b[32mI (338) main_task: Calling app_main()\u001b[0m
\u001b[34mI (348) NvTcpip: Restored TCP/IP configuration (method=Static)\u001b[0m
\u001b[34mI (388) esp_eth.netif.netif_glue: 30:ed:a0:e0:fd:96\u001b[0m
\u001b[34mI (388) esp_eth.netif.netif_glue: ethernet attached to netif\u001b[0m
\u001b[32mI (2588) opener_main: Ethernet Started\u001b[0m
\u001b[32mI (2588) opener_main: Ethernet Link Up\u001b[0m
\u001b[32mI (2588) opener_main: Ethernet HW Addr 30:ed:a0:e0:fd:96\u001b[0m
\u001b[35mIdentity state -> 2\u001b[0m
\u001b[35mSetting extended status: 30\u001b[0m
\u001b[32mI (2588) opener_main: Ethernet Got IP Address\u001b[0m
\u001b[32mI (2588) opener_main: ~~~~~~~~~~~\u001b[0m
\u001b[32mI (2598) opener_main: IP Address: 172.16.82.164\u001b[0m
\u001b[32mI (2598) opener_main: Netmask: 255.255.255.0\u001b[0m
\u001b[32mI (2608) opener_main: Gateway: 172.16.82.1\u001b[0m
\u001b[32mI (2608) opener_main: ~~~~~~~~~~~\u001b[0m
creating class 'message router' with code: 0x2
adding 1 instances to class message router
>>> Allocate memory for meta-message router 1 bytes times 3 for masks
>>> Allocate memory for message router 1 bytes times 3 for masks
meta-message router, number of services:2, service number:1
meta-message router, number of services:2, service number:14
message router, number of services:1, service number:14
creating class 'identity' with code: 0x1
adding 1 instances to class identity
>>> Allocate memory for meta-identity 1 bytes times 3 for masks
>>> Allocate memory for identity 2 bytes times 3 for masks
meta-identity, number of services:2, service number:1
meta-identity, number of services:2, service number:14
identity, number of services:5, service number:14
identity, number of services:5, service number:1
identity, number of services:5, service number:5
identity, number of services:5, service number:3
identity, number of services:5, service number:4
creating class 'TCP/IP interface' with code: 0xF5
adding 1 instances to class TCP/IP interface
>>> Allocate memory for meta-TCP/IP interface 1 bytes times 3 for masks
>>> Allocate memory for TCP/IP interface 2 bytes times 3 for masks
meta-TCP/IP interface, number of services:2, service number:1
meta-TCP/IP interface, number of services:2, service number:14
TCP/IP interface, number of services:3, service number:14
TCP/IP interface, number of services:3, service number:1
TCP/IP interface, number of services:3, service number:16
creating class 'Ethernet Link' with code: 0xF6
adding 1 instances to class Ethernet Link
>>> Allocate memory for meta-Ethernet Link 1 bytes times 3 for masks
>>> Allocate memory for Ethernet Link 2 bytes times 3 for masks
meta-Ethernet Link, number of services:2, service number:1
meta-Ethernet Link, number of services:2, service number:14
Ethernet Link, number of services:3, service number:14
Ethernet Link, number of services:3, service number:1
Ethernet Link, number of services:3, service number:76
creating class 'connection manager' with code: 0x6
adding 1 instances to class connection manager
>>> Allocate memory for meta-connection manager 1 bytes times 3 for masks
>>> Allocate memory for connection manager 2 bytes times 3 for masks
meta-connection manager, number of services:2, service number:1
meta-connection manager, number of services:2, service number:14
connection manager, number of services:8, service number:14
connection manager, number of services:8, service number:1
connection manager, number of services:8, service number:84
connection manager, number of services:8, service number:91
connection manager, number of services:8, service number:78
connection manager, number of services:8, service number:90
connection manager, number of services:8, service number:86
connection manager, number of services:8, service number:87
creating class 'assembly' with code: 0x4
>>> Allocate memory for meta-assembly 1 bytes times 3 for masks
>>> Allocate memory for assembly 1 bytes times 3 for masks
meta-assembly, number of services:1, service number:14
assembly, number of services:2, service number:14
assembly, number of services:2, service number:16
creating class 'Quality of Service' with code: 0x48
adding 1 instances to class Quality of Service
>>> Allocate memory for meta-Quality of Service 1 bytes times 3 for masks
>>> Allocate memory for Quality of Service 2 bytes times 3 for masks
meta-Quality of Service, number of services:2, service number:1
meta-Quality of Service, number of services:2, service number:14
Quality of Service, number of services:2, service number:14
Quality of Service, number of services:2, service number:16
adding 1 instances to class assembly
adding 1 instances to class assembly
adding 1 instances to class assembly
\u001b[35mSetting extended status: 30\u001b[0m
OpENer: opener_thread started, free heap size: 524100
\u001b[35mSetting extended status: 30\u001b[0m
\u001b[34mI (2958) esp_netif_handlers: eth ip: 172.16.82.164, mask: 255.255.255.0, gw: 172.16.82.1\u001b[0m
\u001b[32mI (2958) opener_main: Ethernet Got IP Address\u001b[0m
\u001b[32mI (2968) opener_main: ~~~~~~~~~~~\u001b[0m
\u001b[32mI (2968) opener_main: IP Address: 172.16.82.164\u001b[0m
\u001b[32mI (2968) opener_main: Netmask: 255.255.255.0\u001b[0m
\u001b[32mI (2978) opener_main: Gateway: 172.16.82.1\u001b[0m
\u001b[32mI (2978) opener_main: ~~~~~~~~~~~\u001b[0m
Opener already initialized, skipping
\u001b[35mSetting extended status: 30\u001b[0m
\u001b[34mI (2988) esp_netif_handlers: eth ip: 172.16.82.164, mask: 255.255.255.0, gw: 172.16.82.1\u001b[0m
\u001b[32mI (2588) main_task: Returned from app_main()\u001b[0m
networkhandler: new TCP connection
>>> network handler: accepting new TCP socket: 57 
New highest socket: 57
Entering HandleDataOnTcpSocket for socket: 57
Data received on TCP: 28
Handles data for TCP socket: 57
Register session
TCP reply: send 28 bytes on 57
Entering HandleDataOnTcpSocket for socket: 57
Data received on TCP: 100
Handles data for TCP socket: 57
Send Request/Reply Data
NotifyMessageRouter: routing unconnected message
NotifyMessageRouter: calling notify function of class 'connection manager'
notify: found instance 1
notify: calling ForwardOpen service
ForwardOpen: ConConnID 0, ProdConnID 3183866862, ConnSerNo 232
We have a Non-Null request
We have a Non-Matching request
Received connection path size: 9
key: ven ID 0, dev type 0, prod code 0, major 0, minor 0
classid 4 (assembly)
Configuration instance id 151
assembly: type bidirectional
connection point 150
connection point 100
Resulting PIT value: 256
IO Exclusive Owner connection requested
No PIT segment available
O->T requested size: 38 bytes
Applying implicit run/idle header compensation for consuming data
O->T expect data len 32 (assembly 32, diff 6)
T->O requested size: 34 bytes
T->O expect data len 32 (assembly 32, diff 2)
networkhandler: UDP socket 58
New highest socket: 58
\u001b[35mSetting extended status: 70\u001b[0m
connection manager: connect succeeded
assembleFWDOpenResponse: sending success response
notifyMR: notify function of class 'connection manager' returned a reply
TCP reply: send 90 bytes on 57
\u001b[35mIdentity state -> 3\u001b[0m
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
elapsed time: 110 ms was longer than RPI: 50 ms
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
Starting data length: 38
data length after sequence count: 36
Discarding forced 4-byte run/idle header from originator
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
Starting data length: 38
data length after sequence count: 36
Discarding forced 4-byte run/idle header from originator
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
Starting data length: 38
data length after sequence count: 36
Discarding forced 4-byte run/idle header from originator
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
Starting data length: 38
data length after sequence count: 36
Discarding forced 4-byte run/idle header from originator
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
Starting data length: 38
data length after sequence count: 36
Discarding forced 4-byte run/idle header from originator
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[35mSetting extended status: 60\u001b[0m
\u001b[36mUDP packet to be sent to: 172.16.82.89:2222\u001b[0m
Starting data length: 38
data length after sequence count: 36
Discarding forced 4-byte run/idle header from originator
... (repeated cyclic I/O updates omitted for brevity)
```


