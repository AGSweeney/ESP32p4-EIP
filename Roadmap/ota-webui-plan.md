# OTA & WebUI Implementation Plan

## OTA Update Support
- **ota-partitions**: Update `partitions.csv` (or equivalent) to add `ota_0`, `ota_1`, and `ota_data` entries sized for the current image (≥1 MB each).
- **sdkconfig-ota**: Enable OTA-related Kconfig options (HTTPS client, OTA rollback handling, certificates) in `sdkconfig`.
- **ota-module**: Create `components/ota_manager` with wrapper logic for fetching firmware via HTTP(S), reporting progress, validating, and triggering reboot.
- **main-integration**: Hook OTA trigger into the main app (CLI command, timer, or diagnostic task) and add status logging.

## Web Dashboard
- **http-server**: Enable ESP-IDF HTTP server component; add `components/webui` to serve REST endpoints and static assets.
- **webui-pages**: Build single-page dashboard (HTML/JS/CSS) showing IP configuration, identity attributes, connection state, and live I/O data.
- **webui-io-endpoints**: Expose JSON endpoints for current I/O assemblies; optionally provide POST endpoint to set outputs when a debug flag is enabled.
- **auth-config**: Provide basic auth or token guard (configurable) for write endpoints to avoid accidental use.

## Documentation & Testing
- **docs-update**: Extend `README.md` with OTA/WebUI usage instructions, API endpoints, and security considerations.
- **test-plan**: Add manual/automated tests covering OTA download success/failure, rollback, and WebUI data refresh.

