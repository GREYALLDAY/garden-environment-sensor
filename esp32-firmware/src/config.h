#pragma once
#include <Arduino.h>

#define CONFIG_URL CONFIG_URL1  // Default URL for configuration
#define SERVER_URL SERVER_URL1  // Default URL for server

constexpr const char* CONFIG_URL1 = "http://<SERVER-IP>:8000/api/config";
constexpr const char* CONFIG_URL2 = "http://<SERVER-IP>:8000/api/config";
constexpr const char* SERVER_URL1 = "http://<SERVER-IP>:8000/api/sensor";
constexpr const char* SERVER_URL2 = "http://<SERVER-IP>:8000/api/sensor";


// ===== Compile-time constants (safe in header) =====
#define DEFAULT_SLEEP_SEC 300             // human-friendly fallback (seconds)
constexpr uint32_t CONNECT_TIMEOUT_MS = 10000;   // WiFi HTTP timeouts, etc.
constexpr int INITIAL_RSSI = -999;

// ===== Runtime globals (declare only; define in a .cpp) =====
extern volatile bool netBusy;

extern uint32_t cfg_version;   // last applied config version
extern uint32_t sleep_sec;     // runtime sleep (seconds)
extern uint32_t sleep_ms;      // derived milliseconds

// ===== NVS persistence API =====
// Initialize NVS (Preferences) for config storage
void initNVS();

// Load persisted (sleep_sec, cfg_version). If none present, leaves fallbacks.
void loadFromNVS(uint32_t &sec, uint32_t &ver);

// Persist current (sleep_sec, cfg_version)
void saveToNVS(uint32_t sec, uint32_t ver);
