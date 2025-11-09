# BNO086 ESP-IDF Component

Scaffold for the SparkFun VR IMU Breakout (BNO086). Provides thin I²C helpers and a soft-reset convenience call so higher-level SH-2 packet handling can be implemented in your application.

## Directory Layout

```
components/bno086/
├── CMakeLists.txt
├── include/
│   └── bno086.h
└── bno086.c
```

## Public API

```c
bool bno086_init(bno086_t *dev, i2c_master_dev_handle_t i2c_dev);
esp_err_t bno086_write(bno086_t *dev, const uint8_t *data, size_t length);
esp_err_t bno086_read(bno086_t *dev, uint8_t *buffer, size_t length);
esp_err_t bno086_reset(bno086_t *dev);
esp_err_t bno086_write_packet(bno086_t *dev, uint8_t channel, const uint8_t *payload, size_t length);
esp_err_t bno086_read_packet(bno086_t *dev, uint8_t *channel, uint8_t *buffer, size_t buffer_length, size_t *payload_length);
```

`bno086_write` and `bno086_read` operate on raw HID packets. The packet helpers encode the HID length/channel header for you, making it easier to send SH-2 commands (`channel = 0`) and read report payloads (`channel = 2`). `bno086_reset` issues a soft reset by writing to the command register.

## Example

```c
bno086_t imu;
bno086_init(&imu, i2c_dev);
ESP_ERROR_CHECK(bno086_reset(&imu));

uint8_t buffer[64];
uint8_t channel;
size_t payload_len;
if (bno086_read_packet(&imu, &channel, buffer, sizeof(buffer), &payload_len) == ESP_OK && payload_len > 0) {
    // buffer[0..payload_len-1] now holds the HID payload for `channel`
}
```

### Rotation Vector → Euler Angles

1. Enable the rotation vector report (ID `0x05`) by sending a *Set Feature Command* on the control channel (`BNO086_CHANNEL_COMMAND`). The payload below enables the report at 100 Hz:

    ```c
    // Set Feature Command (report 0xFD)
    uint8_t enable_rotation_vector[] = {
        0xFD,       // Report ID: Set Feature Command
        0x05,       // Feature: Rotation Vector
        0x00,       // Feature flags
        0x00, 0x00, 0x00, 0x00, // Change sensitivity (ignored)
        0x10, 0x27, 0x00, 0x00, // Report interval = 0x00002710 (100 Hz)
        0x00, 0x00              // Specific config
    };

    ESP_ERROR_CHECK(bno086_write_packet(&imu,
                                        BNO086_CHANNEL_COMMAND,
                                        enable_rotation_vector,
                                        sizeof(enable_rotation_vector)));
    ```

2. When the rotation vector report arrives on the report channel, parse it with `bno086_parse_rotation_vector` and convert to Euler angles:

    ```c
    uint8_t report_payload[64];
    uint8_t report_channel;
    size_t report_length;

    if (bno086_read_packet(&imu,
                           &report_channel,
                           report_payload,
                           sizeof(report_payload),
                           &report_length) == ESP_OK &&
        report_channel == BNO086_CHANNEL_REPORTS)
    {
        bno086_rotation_vector_t quat = {0};
        bno086_euler_t euler = {0};
        if (bno086_parse_rotation_vector(report_payload, report_length, &quat) == ESP_OK)
        {
            bno086_rotation_vector_to_euler(&quat, &euler);
            const float RAD_TO_DEG = 57.2957795f;
            ESP_LOGI("BNO086", "Euler roll=%.2f pitch=%.2f yaw=%.2f deg (status=0x%02X)",
                     euler.roll * RAD_TO_DEG,
                     euler.pitch * RAD_TO_DEG,
                     euler.yaw * RAD_TO_DEG,
                     quat.status);
        }
    }
    ```

The quaternion elements are normalised floats (`w`, `x`, `y`, `z`) and the optional accuracy field is reported in radians. Convert the angles to degrees if desired.

## Notes

- SparkFun ships the board configured for I²C address 0x4A. The BNO086 also supports SPI.
- After reset, load the latest SH-2 sensor hub firmware image if required by your application. Hillcrest Labs (now CEVA) provides the official firmware packages; SparkFun mirrors the latest builds alongside their Arduino driver. Grab the image from either of these sources and stream it over I²C using the packet helpers above:
  - CEVA/Hillcrest Labs GitHub (official SH-2 firmware + tools): <https://github.com/ceva-dsp/sh2>
  - SparkFun BNO080/BNO086 Arduino library (prepackaged firmware blobs under `src/util/`): <https://github.com/sparkfun/SparkFun_BNO080_Arduino_Library>

  The firmware files are typically named `BNO08X_VR_*.bin` (VR/IMU builds). Load the one that matches your target features (e.g., VR, AR, game rotation). For production deployments, check CEVA’s release notes to confirm compatibility with your hardware revision.
- Configure report intervals, sensor enable masks, and calibration through SH-2 commands layered above this component.

