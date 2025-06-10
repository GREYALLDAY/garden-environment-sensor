# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### To be Added
- [TESTING] deep sleep mode for systems running on smaller batteries
- ntfy.sh notification support
- `/api/config` endpoint to modify/retrieve ESP32 configuration settings
- allow POST based updates such as changes to:
  - wi-fi credentials
  - mqtt topic names
  - sensor polling rate
- Refined dashboard UI with improved mobile support
- Offline SD card logging support
- Update fastapi.py with changes made to dashboard.py for better compatiability
- BMP280 barometric sensor support
- `/api/export` endpoint to download current log

### To be Changed
- Improve error logging/handling, add more edge cases
- Custom thresholds for light/moisture/humidity/pressure
- Push MQTT/ntfy.sh alert when threshold is exceeded
- Historical zoom + pan on graph
- add uptime counter to `/api/status`

### To be Fixed
- UTC to CST time conversion and logging still messy
- Add support for other time zones, ability switch between time zones

## [1.1.1] - 2025-06-10
### Added
- `Gunicorn` support added for `Flask` deployment options, codebase and behavior remains the same
- Added `gunicorn.config.py` for easier configurability (see README for instructions on how to set up)
- NTP timestamping
- ISO 8601 UTC time formatting
- `data-cleaner.py` utility script to filter and clean raw sensor data
- Replaced `paho_mqtt` with `flask_mqtt`
- OTA support - no more hiking out to the garden every time you want to change a line of code!
- More explanatory comments, robust debug statements so you know exactly what is going on
- Error, access logging for flask server, stored in `garden-dev/dashboard/flask/logs` 
- `deep-sleep.cpp` added, still in testing phase, feel free to try it out but may be buggy still
### Changed
- Swapped BH1750 light sensor with OPT3001, as its more accurate especially reading higher light levels.
- Changed folder structure layout `flask-dashboard` is now `flask`
- Renamed `flask-dashboard.py` to `dashboard.py`
- Cleaning/standardizing logging sensor data to `raw_sensorlog.csv`
- Header creation, data validation for `raw_sensorlog.csv`
- Recalibrated moisture sensor values
- Improved light sensor read timings, power optimization
- Improved network resilience, reliability
- Comments on OPT3001 configuration
- Changed from `delay()` to non blocking delay interval so ESP is free to handle other tasks in between readings such as OTA updates

### Fixed
- issue with sensor readings not being logged if `raw_sensorlog.csv` did not contain the correct headers
- added logic to test if file exists, is empty, if both are true headers will be added to file
- added a 2nd Y axis for Lux readings to better scale on the UI graph, higher Lux values (1000+) caused the other readings to get squished.

## [1.1.0] - 2025-05-30
### Added
- MQTT integration with `paho-mqtt` to publish real-time sensor data to `garden/sensors` topic
- `api/sensor` POST route now publishes JSON payloads to MQTT with `retain=True`
- Added persistent MQTT client with connection started at server init
- Implemented graceful MQTT shutdown using `atexit.register`
### Changed
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
