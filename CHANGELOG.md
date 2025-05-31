# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- [COMPLETE] MQTT publishing support for Home Assistant integration
- [IN PROGRESS] `/api/config` endpoint to retrieve ESP32 Wi-Fi/network settings
- Optional push notification support via ntfy.sh

### Changed
- Refined dashboard UI with better mobile/responsive support
- Refactored ESP32 firmware for clarity and modularity

### Fixed
- Resolved Wi-Fi reconnection logic on multi-network switches
- Corrected timestamp formatting in CSV for better cross-region compatibility

## [1.1.0] - 2025-05-30
### Added
- MQTT integration with `paho-mqtt` to publish real-time sensor data to `garden/sensors` topic
- `api/sensor` POST route now publishes JSON payloads to MQTT with `retain=True`
- Added persistent MQTT client with connection started at server init
- Implemented graceful MQTT shutdown using `atexit.register`
### Improved
- Updated `/api/sensor` POST logic with strong JSON validation and float conversion
- Change log timestamps to ISO format for better machine parsing
- Status/error handling:
  * `204` returned when no sensor data exists
  * `400` returned for malformed or missing payloads
  * `201` returned on successful submission
### Fixed
- CSV header logic to ensure headers are written only once on file creation
- Corrected `latest_data` handling to avoid `NoneType` errors



## [1.0.0] - 2025-05-30
### Added
- Initial stable version of the Garden Environment Sensor
- Flask server (`/api/sensor`, `/api/history`, `/dashboard`)
- FastAPI alternative with identical endpoints
- Front-end dashboard using HTML/CSS + Chart.js for visualizing sensor data
- ESP32 firmware that:
  - Connects to multiple Wi-Fi networks
  - Sends JSON data via HTTP POST
- Local CSV data logging (`sensor_log.csv`)
- `requirements.txt` for Python dependencies
- MIT License
- Comprehensive README with setup and install instructions

### Changed
- Reorganized folder structure for clarity
- Updated dashboard template structure for easier editing

### Removed
- Cleaned up hardcoded credentials and sensitive data

## [0.1.0] - 2025-05-30
Initial version with:
- Basic sensor reading
- Simple HTTP API (Flask)
- CSV-based data logging
