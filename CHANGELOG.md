# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- MQTT publishing support for Home Assistant integration
- `/api/config` endpoint to retrieve ESP32 Wi-Fi/network settings
- Optional push notification support via ntfy.sh

### Changed
- Refined dashboard UI with better mobile/responsive support
- Refactored ESP32 firmware for clarity and modularity

### Fixed
- Resolved Wi-Fi reconnection logic on multi-network switches
- Corrected timestamp formatting in CSV for better cross-region compatibility

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
