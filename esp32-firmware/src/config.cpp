#include "config.h"

#if defined(ESP32)
  #include <Preferences.h>
  static Preferences prefs;
#endif

// ===== Define globals exactly once =====
volatile bool netBusy = false;

uint32_t cfg_version = 0;
uint32_t sleep_sec   = DEFAULT_SLEEP_SEC;
uint32_t sleep_ms    = DEFAULT_SLEEP_SEC * 1000UL;

// ===== NVS keys / namespace =====
static constexpr const char* NVS_NS        = "cfg";
static constexpr const char* KEY_SLEEP_SEC = "sleep_sec";
static constexpr const char* KEY_SLEEP_MS  = "sleep_ms";   // legacy (ms) for migration
static constexpr const char* KEY_CFG_VER   = "cfg_ver";

// ===== NVS helpers =====
void initNVS() {
#if defined(ESP32)
  prefs.begin(NVS_NS, /*readOnly=*/false);
#endif
}

void loadFromNVS(uint32_t &sec, uint32_t &ver) {
#if defined(ESP32)
  // Try new seconds key first
  if (prefs.isKey(KEY_SLEEP_SEC)) {
    sec = prefs.getUInt(KEY_SLEEP_SEC, DEFAULT_SLEEP_SEC);
  } else if (prefs.isKey(KEY_SLEEP_MS)) {
    // One-time migration from old milliseconds key
    uint32_t old_ms = prefs.getUInt(KEY_SLEEP_MS, DEFAULT_SLEEP_SEC * 1000UL);
    sec = (old_ms + 500) / 1000;                         // round to nearest second
    if (sec < 1) sec = 1;
    if (sec > 6000) sec = 6000;                          // clamp to new range
    prefs.putUInt(KEY_SLEEP_SEC, sec);
    prefs.remove(KEY_SLEEP_MS);
  } else {
    sec = DEFAULT_SLEEP_SEC;
  }

  ver = prefs.getUInt(KEY_CFG_VER, 0);

  // Reflect into globals too (convenience)
  sleep_sec = sec;
  sleep_ms  = sec * 1000UL;
  cfg_version = ver;
#else
  // Non-ESP32: leave fallbacks
  (void)sec; (void)ver;
#endif
}

void saveToNVS(uint32_t sec, uint32_t ver) {
#if defined(ESP32)
  prefs.putUInt(KEY_SLEEP_SEC, sec);
  prefs.putUInt(KEY_CFG_VER, ver);
#endif
  // Keep globals consistent
  sleep_sec = sec;
  sleep_ms  = sec * 1000UL;
  cfg_version = ver;
}
