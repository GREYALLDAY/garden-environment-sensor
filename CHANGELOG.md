# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### To be Added
- Planned: `/api/export` endpoint to download current log (experimental)
- Planned: Offline SD card logging support (experimental)
- Planned: Battery voltage monitor
- Planned: Machine Learning/Inference support (experimental)



### To be Changed
- Expect changes in `main.cpp` in coming updates, planning to build out more custom headers as well as further modularize the source code for better maintainability.
- File layout for the server will also experience breaking changes as I make improvements to the server, data/error handling and validation, etc



### To be Fixed
- Improve pan/zoom on dashboard graph

[1.3.0] - 2025-08-18
## TL;DR 
- Major changes to the structure of dashboard.py, split into modules for better maintainability and readability
- Major improvements to `dashboard.html`, time filtering, graph pan/zoom, overall performance
- Hardened OTA updates, fixed race conditions
- Added `config.json` configuration file to server config, this is the source of truth
- Added fallback values if `config.json` can't be pulled from server
- Added .env to server to store sensitive information (API tokens, ntfy server URL, etc)
- Added secrets.h to store hardcoded credentials
- Improved log/error handling in server
- General performance improvements to ESP source code

### Added [dashboard.html]
- Time-series x-axis using Chart.js time scale with x: Date, y: value points for all datasets.
- Zoom & pan via chartjs-plugin-zoom:
- Mouse wheel + pinch zoom on X.
- Ctrl+drag pan on X.
- resetZoom() global.
- Quick range filters: buttons to set X-axis to:
- Last hour (filterRange(1))
- Last 24h (filterRange(24))
- Last 7 days (filterRange(168))

#### Changed [dashboard.html]
- Data ingestion:
  - Was: separate labels array + numeric arrays.
  - Now: per-dataset { x: Date(entry.timestamp), y: value } points.
- Averages:
  - Was: computed from raw arrays.
  - Now: computed from map(p => p.y) across point arrays (functionally same, structurally updated).
- Chart options:
  - X scale now type: 'time' with tooltip format and default unit hour.
- Added interaction: { mode: 'nearest', intersect: false } for better hover behavior.
- DOM:
  - Added a small controls bar (Reset Zoom / range filter buttons) under the chart.

### Fixed / Improved [dashboard.html]
- Dual-axis intent clearer: y1 for Temp/Humidity/Moisture, y2 for Lux still intact with grid.drawOnChartArea=false on y2 to reduce clutter.
- Legend readability: consistent font sizing and color maintained.
- Potential Issues / Breaking Notes (read this)
- Time scale adapter required: Chart.js v3+ needs a date adapter for the time scale. You did not include one. Without it, the x-axis will fall back or misbehave.

### Removed [dashboard.html]
- Labels array on the chart (now implicit via point timestamps).


### Added [Server]
- Modular architecture — Split monolithic dashboard.py into focused modules for clarity and testability:
  - routes.py (Blueprint with all API/UI routes)
  - mqtt_handler.py (centralized publish_mqtt(topic, payload))
  - ntfy_handler.py (optional push notifications)
  - logging_config.py (app + module loggers, rotation)
  - settings.py (paths and constants: CONFIG_FILE, LOG_DIR, RAW_LOG_FILE, etc.)
- Config API as source of truth — GET /api/config now serves a standardized config.json for ESP devices (sleep, mqtt_broker, mqtt_port, updated_at, ...).
- Stronger input validation — Centralized checks for required sensor fields and types before write/publish.
- Error + info log separation — Distinct rotating files for app info and errors; consistent timestamps and formats.
- .env support — Load broker/ntfy and other secrets via environment with sane defaults (dotenv).

### Changed [Server]
- Blueprinted routing — Routes registered via a Blueprint; dashboard.py is now the entry point only.
- Serialization & status shape — Standardized {"status":"ok","data":...} and {"status":"error","message":...} across endpoints.
- CSV history pipeline — Hardened `/api/history` parsing (ISO8601), stable numeric coercion, sorted output, and safer header handling.
- Config handling — `load_config()`/`save_config()` now guard against malformed JSON and write atomically; unknown keys ignored or defaulted.
- MQTT publish — Publishing moved out of the request function into `mqtt_handler.py` with centralized broker settings (host/port/auth).
- Templates/Static — `TEMPLATES_AUTO_RELOAD=True` retained; paths consolidated via `settings.py`.
- Performance — Reduced blocking work inside request handlers; fewer direct print()s (replaced with logging).

### Fixed [Server]
- `/api/history` crash — Resolved str object cannot be interpreted as an integer by enforcing numeric window params and explicit datetime parsing.
- Empty/partial CSV writes — Ensured header presence and consistent row formatting; skipped corrupt rows with trace logging.
- MQTT publish robustness — Exceptions no longer crash the request path; errors are logged and surfaced as JSON.

### Removed [Server]
- Monolithic logic blocks and inline MQTT code in `dashboard.py`.
- Ad-hoc console prints in favor of structured logging.
- Inline VEML debug file init scattered in route code (now centralized).

### Breaking Changes [Server]
- Config schema — `config.json` is now authoritative for ESP and uses sleep (seconds), mqtt_broker (string), mqtt_port (int), updated_at (ISO8601). If you relied on `polling_rate/ssid/password` from the old schema, update clients.
- Log locations — Logs are under `./logs/` (see `settings.py`). Update any log shippers/collectors accordingly.
- Response shapes — Some endpoints now return the standardized status/data envelope; consumers parsing raw arrays should adjust.
- Ensure `logs/` is writable by the app user.
- Provide a `.env` (or environment vars) for MQTT_HOST, MQTT_PORT, and NTFY_* if you’re using notifications.
- Behind a reverse proxy, bump body size limits if you plan to POST batched sensor data.

### Added [ESP]
- RTC Wake Optimization – Implemented RTC support to avoid full network scans after deep sleep. ESP now stores last known good network credentials and attempts a direct reconnect before falling back to a scan, reducing power consumption.
- SHT31 Sensor Support – Added temperature/humidity readings from SHT31.
- Dynamic Configuration – ESP now retrieves configuration from `/api/config` `(config.json)` as the primary source of truth. If unavailable or invalid, it falls back to hardcoded defaults in `config.h`.
- Automatic Light Zone Switching (WIP) – New `VEML7700_Enhanced.h` header handles gain and integration time adjustments based on ambient light levels, improving accuracy in both low-light and bright environments.
- New `vemlCtrl()` Handler – Interface function for interacting with the Enhanced VEML7700 driver.
- Configuration Management System – `config.cpp` now supports Non-Volatile Storage (NVS) to persist settings across reboots.
- Sleep Duration Validation – Added `valid_secs()` to clamp configurable sleep duration between 1–6000 seconds.
- Refactored `fetchConfig()` – Improved timeout/error handling, introduced fallback logic, and updated ArduinoJson usage to v7.

### Changed [ESP]
- Improved deep sleep entry – `goToSleep()` now fully shuts down Wi-Fi before sleeping.
- Moved hardcoded credentials to `secrets.h`.
- Relocated VEML configuration to `config.h`.
- Enhanced OTA handling for better network state awareness.
- Replaced blocking `delay()` calls with non-blocking `vTaskDelay()`.
- Refined Wi-Fi reconnection/timeout logic in `connectWiFi()`.
- syncTime() now uses `time.h`, with better timeout handling and NVS persistence.
- Improved `initSensors()` error handling.
- Increased soil moisture scale to 150% to better detect overwatering.
- Improved configuration persistence in NVS after `fetchConfig()`.
- Refined JSON serialization/deserialization, error handling, and network handling in `sensorTask()`.
- Moved sensor variants (`opt3001.cpp`, `hdc1000.cpp`, etc) into `variants/`

### Removed [ESP]

- Removed `wifiMonitorTask()` – superseded by updated `connectWiFi()` reconnection logic and deep sleep handling.

## [1.2.0] - 2025-06-25
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
