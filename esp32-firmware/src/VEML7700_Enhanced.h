#ifndef VEML7700_ENHANCED_H
#define VEML7700_ENHANCED_H

#include <Arduino.h>
#include <Adafruit_VEML7700.h>

// Four zones
#define VEML_ZONE_VERY_DIM   0
#define VEML_ZONE_DIM        1
#define VEML_ZONE_BRIGHT     2
#define VEML_ZONE_VERY_BRIGHT 3
#define VEML_ZONE_COUNT      4

// Back-compat aliases (if anything still references these)
#define VEML_ZONE_LOW   VEML_ZONE_VERY_DIM
#define VEML_ZONE_HIGH  VEML_ZONE_VERY_BRIGHT

extern int rtc_currentZone;
extern int rtc_confirmCount;

class VEML7700_Enhanced {
public:
  explicit VEML7700_Enhanced(Adafruit_VEML7700* sensor)
  : veml(sensor), currentZone(VEML_ZONE_VERY_BRIGHT), lastLux(0) {}

  bool begin() {
    float lux = averageLux_();
    uint8_t target = pickZone_(lux);
    applyZone_(target, /*force=*/true);   // always apply once per boot
    currentZone = target;
    rtc_currentZone  = currentZone;
    rtc_confirmCount = 0;
    lastLux = veml->readLux();
#ifdef VEML_DEBUG
    Serial.printf("[VEML_BOOT] Zone=%u, Gain=%.2f, IT=%ums, Lux=%.1f\n",
                  currentZone, getCurrentGain(), getIT(), lastLux);
#endif
    return true;
  }

  // Call once per cycle before you read Lux
  bool update() {
    float lux = averageLux_();
    uint8_t target = pickZone_(lux);

    if (target != currentZone) {
      applyZone_(target);              // guarded (no force here)
      currentZone       = target;
      rtc_currentZone   = currentZone;
      rtc_confirmCount  = 0;
#ifdef VEML_DEBUG
      Serial.printf("[VEML_DEBUG_UPDATE_1] Zone -> %u\n", currentZone);
#endif
      // fresh reading after change
      lux = veml->readLux();
      lastLux = lux;
#ifdef VEML_DEBUG
      Serial.printf("[VEML_DEBUG_UPDATE_2] Lux=%.1f Zone=%u\n", lastLux, currentZone);
#endif
      return true; // changed
    }

    lastLux = lux;
#ifdef VEML_DEBUG
    Serial.printf("[VEML_DEBUG_UPDATE_2] Lux=%.1f Zone=%u\n", lastLux, currentZone);
#endif
    return false; // no change
  }

  // Accessors used by your task
  float   getALS()        { return veml->readALS(); }
  float   getWhite()      { return veml->readWhite(); }
  float   getLux()  const { return lastLux; }
  uint8_t getZone() const { return currentZone; }
  float   getCurrentGain() const { return veml->getGainValue(); }
  uint16_t getIT()   const { return veml->getIntegrationTimeValue(); }

  // Optional setters (no-ops for confirmations; kept for compatibility)
  void setZone(uint8_t z) { applyZone_(z, /*force=*/true); currentZone = z; }
  void setConfirm(uint8_t /*cnt*/) {}

private:
  // Thresholds (ordered high→low check fills your gaps deterministically)
  static constexpr float TH_VERY_BRIGHT = 25000.0f;
  static constexpr float TH_BRIGHT      =  5000.0f;
  static constexpr float TH_DIM         =  1500.0f;

  Adafruit_VEML7700* veml;
  uint8_t            currentZone;
  float              lastLux;

  static uint16_t itToMs_(uint8_t it) {
    switch (it) {
      case VEML7700_IT_25MS:  return 25;
      case VEML7700_IT_50MS:  return 50;
      case VEML7700_IT_100MS: return 100;
      case VEML7700_IT_200MS: return 200;
      case VEML7700_IT_400MS: return 400;
      case VEML7700_IT_800MS: return 800;
      default: return 100;
    }
  }

  uint8_t pickZone_(float lux) const {
    if (lux >= TH_VERY_BRIGHT) return VEML_ZONE_VERY_BRIGHT;
    if (lux >= TH_BRIGHT)      return VEML_ZONE_BRIGHT;
    if (lux >= TH_DIM)         return VEML_ZONE_DIM;
    return VEML_ZONE_VERY_DIM;
  }

  void applyZone_(uint8_t z, bool force = false) {
    uint8_t from = currentZone;
    if (!force && z == currentZone) return;

    // Settings per zone (tweak as needed)
    uint8_t gain, it;
    switch (z) {
      case VEML_ZONE_VERY_BRIGHT: gain = VEML7700_GAIN_1_8; it = VEML7700_IT_25MS;  break;
      case VEML_ZONE_BRIGHT:      gain = VEML7700_GAIN_1_4; it = VEML7700_IT_50MS;  break;
      case VEML_ZONE_DIM:         gain = VEML7700_GAIN_1;   it = VEML7700_IT_200MS; break;
      case VEML_ZONE_VERY_DIM:    gain = VEML7700_GAIN_2;   it = VEML7700_IT_400MS; break;
      default:                    gain = VEML7700_GAIN_1;   it = VEML7700_IT_100MS; break;
    }

    veml->setGain(gain);
    veml->setIntegrationTime(it);
    delay(itToMs_(it));
    (void)veml->readLux(); // discard first

#ifdef VEML_DEBUG
    Serial.printf("[VEML_DEBUG_APPLYZONE] %u -> %u  Gain=%.2f IT=%ums\n",
                  from, z, veml->getGainValue(), getIT());
#endif
  }

  float averageLux_() {
    float s = 0;
    for (uint8_t i = 0; i < 3; ++i) { s += veml->readLux(); delay(5); }
    return s / 3.0f;
  }
};

#endif // VEML7700_ENHANCED_H
