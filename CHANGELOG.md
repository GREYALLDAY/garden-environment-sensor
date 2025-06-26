# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### To be Added
- Planned: `/api/export` endpoint to download current log (experimental)
- Planned: ntfy.sh notification logic (experimental)
- Planned: Offline SD card logging support (experimental)
- Planned: Battery voltage monitor
- Planned: Machine Learning/Inference support (experimental)
- Testing: BMP280 barometric sensor support (testing integration)
- Testing: Sensor data smoothing via sample averaging per wake cycle (testing integration)


### To be Changed

- Custom thresholds for light/moisture/humidity/pressure
- Historical zoom + pan on graph
- add uptime counter to `/api/status`

### To be Fixed
- Dashboard date layout is currently too crowded after collecting many data points, working on a more elegant fix

## [1.1.2] - 2025-06-25
Major firmware overhaul with FreeRTOS task-based design, deep sleep integration, OTA improvements, and memory leak fixes.

### Added
- Added deep sleep support using `esp_deep_sleep()`
  - Drastically increases battery life, wakes only to take sensor readings and send to server.
- Refactored codebase for FreeRTOS support, no more loops, only tasks!
  - `sensorTask` -> takes sensor readings, sends POST, then calls goToSleep()
  - `initSensors` -> initializes and enables sensors
  - `goToSleep()` -> function to handle resetting wdt, entering deep sleep
  - `wifiMonitorTask` -> Checks WiFi connection, sends reconnect request when disconnected
  - `OTATask` -> Checks for OTA requests, pauses sleep if OTA request received 
  - `connectConfig()` -> parses config.json for credentials, attempts to connect to network, if fails     will fallback to hardcoded network.
- Added VEML7700 light sensor support
- Added ESP32-S3 support
### Changed
- Updated platformio.ini with proper build flags for ESP32 Dev Board and ESP32-S3
- Fixed HTTP memory leak caused by missing `http.end()` after POST requests
- Fixed memory leak leak from hanging `for` loop in `sensorTask`
- Improved WiFi fallback/reconnection logic
  - Multi-network fallback support
- Improved code readability
  - Added more [DEBUG] lines for additional information such as Free Heap
- Improved OTA logic, put into its own function for easier reuse
  - Credentials now entered once at the top of the codeblock, 
  - Improved dynamic network handling
- Upgraded from delay() to non blocking delays
- `esp_task_wdt` watchdog timer for more robust handling of errors and hangs
### Removed
- Removed `loop ()` logic, everything now runs in FreeRTOS Tasks, this increases performance by now running tasks in parallel, utilizing both cores
- Removed and replaced much of the `setup()` logic with predefined functions for better modularity

### [Changes] (dashboard.py)

#### Added

- **API Enhancements**
  - `POST /api/sensor` now logs to:
    - `logs/raw_sensorlog.csv` — general sensor data
    - `logs/veml-debug.csv` — VEML7700-specific debug log (ALS, WHITE, integration time, gain)
  - `GET /api/sensor` returns the most recent reading (`latest_data`)
  - `GET /api/history` now returns cleaned, timestamp-formatted historical data

- **MQTT Integration**
  - Sensor payloads are published to `garden/sensors` topic
  - MQTT messages are JSON encoded and sent with `retain=True`

- **Configuration API**
  - `GET /api/config` returns stored config from `config.json`
  - `POST /api/config` allows config updates with basic type validation and value checks

#### Improved

- **Logging System**
  - Rotating file log at `logs/sensor-log.log` (max size 1MB, 5 backups)
  - Console logs with standardized timestamp and level formatting

- **Dashboard Integration**
  - Template and static paths are now handled cleanly from `dashboard.py`

- **API Robustness**
  - All APIs now validate JSON payloads and return helpful error messages
  - Auto-creates `logs/` directory if missing to avoid runtime errors
  - Timestamp handling standardized (UTC ISO 8601 format)


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
