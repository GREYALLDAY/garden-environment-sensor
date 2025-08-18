#define VEML_DEBUG
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <time.h>
#include <Adafruit_SHT31.h>
#include "Adafruit_VEML7700.h"
#include "esp_task_wdt.h"
#include <esp_wifi.h>

#include "secrets.h"   
#include "config.h"    // DEFAULT_SLEEP_SEC, CONNECT_TIMEOUT_MS, extern globals, NVS helpers
#include "VEML7700_Enhanced.h"

#if defined(BOARD_ESP32)
  #define SDA 21
  #define SCL 22
  #define MOISTURE_PIN 35
#elif defined(BOARD_ESP32S3)
  #define SDA 8
  #define SCL 9
  #define MOISTURE_PIN 4
#endif

#define uS_TO_S_FACTOR 1000000ULL // microseconds → seconds

// ---------- Sensors ----------
Adafruit_SHT31 sht31;
Adafruit_VEML7700 veml;
VEML7700_Enhanced vemlCtrl(&veml);

// ---------- RTC-stored state ----------
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool rtc_scanned = false;
RTC_DATA_ATTR uint8_t rtc_networkIndex = 0;
RTC_DATA_ATTR int rtc_currentZone = VEML_ZONE_BRIGHT;
RTC_DATA_ATTR int rtc_confirmCount = 0;

// ---------- OTA state ----------
static bool otaStarted = false;
volatile bool otaInProgress = false;

// ---------- WiFi creds ----------
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};
WiFiCredentials knownNetworks[] = {
  { SSID1, PASSWORD1 }, { SSID2, PASSWORD2 }
};
const int numKnownNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

// ---------- Helpers ----------
static inline bool valid_secs(uint32_t s) { return s >= 1 && s <= 6000; }

bool isDeepSleepWakeup() {
  return esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED;
}

void goToSleep() {
  Serial.println("Going to sleep...");
  WiFi.persistent(true);
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  esp_task_wdt_reset();
  vTaskDelay(pdMS_TO_TICKS(100));
  // Use the source-of-truth seconds
  esp_sleep_enable_timer_wakeup((uint64_t)sleep_sec * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

bool connectWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Already connected to %s (IP: %s)\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    return true;
  }

  auto try_connect = [&](const char* ssid, const char* pass)->bool {
    uint32_t startAttempt = millis();
    WiFi.begin(ssid, pass);
    Serial.printf("\nConnecting to %s", ssid);
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < CONNECT_TIMEOUT_MS) {
      vTaskDelay(pdMS_TO_TICKS(500));
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\nConnected! IP: %s, RSSI: %d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      return true;
    }
    Serial.println(F("\nWiFi connection failed."));
    return false;
  };

  // Try last known (if deep-sleep resume)
  if (isDeepSleepWakeup() && rtc_scanned && rtc_networkIndex < numKnownNetworks) {
    if (try_connect(knownNetworks[rtc_networkIndex].ssid, knownNetworks[rtc_networkIndex].password)) return true;
    rtc_scanned = false; // reset, we'll rescan
  }

  Serial.print(F("Scanning for networks..."));
  int networksFound = WiFi.scanNetworks();
  if (networksFound <= 0) {
    Serial.println(F("No wifi networks found!"));
    return false;
  }

  int bestIndex = -1;
  int bestRSSI = INITIAL_RSSI;
  for (int j = 0; j < networksFound; j++) {
    for (int i = 0; i < numKnownNetworks; i++) {
      if (WiFi.SSID(j) == knownNetworks[i].ssid && WiFi.RSSI(j) > bestRSSI) {
        bestRSSI = WiFi.RSSI(j);
        bestIndex = i;
      }
    }
  }
  if (bestIndex == -1) {
    Serial.println(F("No known networks found!"));
    return false;
  }
  rtc_networkIndex = bestIndex;
  rtc_scanned = true;
  return try_connect(knownNetworks[bestIndex].ssid, knownNetworks[bestIndex].password);
}

void setupOTA() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("garden-sensor");
  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    Serial.println("\nStart OTA Update");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update complete.");
    otaInProgress = false;
    ESP.restart();
  });
  ArduinoOTA.onError([](ota_error_t e){
    Serial.printf("OTA error: %u\n", e);
    otaInProgress = false;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("\nProgress:  %u%%\r", (progress / (total / 100 )));
  });
  if (!otaStarted) {
    ArduinoOTA.begin();
    otaStarted = true;
    vTaskDelay(pdMS_TO_TICKS(250));
    Serial.print(F("OTA Available\n"));
  }
}

void connectWiFi_OTA() {
  if (connectWiFi()) {
    setupOTA();
  } else {
    Serial.print(F("[OTA] Connection failed. OTA not available.\n"));
  }
}

void syncTime() {
  setenv("TZ", "CST6CDT,M3.2.0/2,M11.1.0/2", 1); tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  uint32_t start = millis();
  Serial.print(F("Waiting for time sync..."));
  while (time(nullptr) < 100000 && millis() - start < 15000) {
    vTaskDelay(pdMS_TO_TICKS(250));
    Serial.print(".");
  }
  Serial.println("\nTime synced!");
}

void initSensors() {
  if (!sht31.begin()) {
    Serial.print(F("[ERROR] Temp sensor init failed.\n"));
    return;
  }
  if (!veml.begin()) {
    Serial.print(F("[ERROR] Light sensor init failed.\n"));
    return;
  }
  vemlCtrl.begin();
  Wire.setClock(400000);
  delay(10);
}

int readMoisturePercent() {
  int minVal = 1200;    // Lower-threshold (adjust per sensor)
  int maxVal = 4000;    // Upper-threshold
  int rawVal = analogRead(MOISTURE_PIN);
  int moisturePercent = map(rawVal, minVal, maxVal, 0, 150);
  moisturePercent = constrain(moisturePercent, 0, 150);
  return moisturePercent;
}




bool fetchConfig() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  netBusy = true;
  http.setTimeout(10000);
  http.begin(CONFIG_URL);
  http.addHeader("Accept", "application/json");
  Serial.printf("Fetching configuration...");

  int code = http.GET();
  if (code < 0) { Serial.printf("[ERROR] HTTP GET failed: %s\n", http.errorToString(code).c_str()); http.end(); netBusy=false; return false; }
  if (code != 200){ Serial.printf("[ERROR] HTTP %d fetching config, falling back to defaults.\n", code); http.end(); netBusy=false; return false; }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end(); netBusy = false;
  if (err) { Serial.printf("[ERROR] Unable to parse config.json: %s\n", err.c_str()); return false; }

  // DEBUG: show whole response once
  serializeJson(doc, Serial); Serial.println();

  // --- Unwrap "data" safely and extract as integers explicitly ---
  JsonObject payload = doc["data"].isNull()
                       ? doc.as<JsonObject>()
                       : doc["data"].as<JsonObject>();

  // sleep (seconds)
  uint32_t new_sec = sleep_sec;
  if (payload["sleep"].is<uint32_t>()) {
    new_sec = payload["sleep"].as<uint32_t>();   // explicit type set
  } else if (payload["polling_interval"].is<uint32_t>()) {
    uint32_t v = payload["polling_interval"].as<uint32_t>();
    if (v > 6000) v /= 1000;                     // ms -> s (back-compat)
    new_sec = v;
  }
  // keeps values between 1 and 6000 seconds
  if (!(new_sec >= 1 && new_sec <= 6000)) {
    Serial.printf("[ERROR] Invalid sleep=%u (expected 1..6000s); keeping %us\n", new_sec, sleep_sec);
    return false;
  }

  // version (explicit)
  uint32_t new_ver = payload["config_version"].is<uint32_t>()
                   ? payload["config_version"].as<uint32_t>()
                   : cfg_version;

  Serial.printf("[INFO] parsed sleep=%u, version=%u (current sleep=%u, ver=%u)\n",
                new_sec, new_ver, sleep_sec, cfg_version);

  bool changed = (new_sec != sleep_sec) || (new_ver > cfg_version);
  if (!changed) {
    Serial.printf("[INFO] Config unchanged (ver=%u, sleep=%us)\n", cfg_version, sleep_sec);
    return true;
  }

  // APPLY + persist
  sleep_sec   = new_sec;
  sleep_ms    = new_sec * 1000UL;
  cfg_version = new_ver ? new_ver : (cfg_version + 1);
  Serial.printf("[INFO] Sleep set to %u seconds (ver=%u)\n", sleep_sec, cfg_version);
  saveToNVS(sleep_sec, cfg_version);
  return true;
}







// -------- Tasks --------
void sensorTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  esp_task_wdt_reset();

  float humidity = sht31.readHumidity();
  float tempC = sht31.readTemperature();
  float tempF = (tempC * 9.0f / 5.0f) + 32.0f;
  float moisture = readMoisturePercent();
  vemlCtrl.update();
  float lux = vemlCtrl.getLux();
  float raw_als = vemlCtrl.getALS();
  float raw_white = vemlCtrl.getWhite();
  int zone = vemlCtrl.getZone(); (void)zone; // not currently posted

  Serial.printf("[INFO] Temp(F): %.2f, Humidity: %.2f, Lux: %.2f, Moisture: %.2f\n",
                tempF, humidity, lux, moisture);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[SENSORTASK_DEBUG] RSSI: %d dBm\n", WiFi.RSSI());
  }

  // Pick server by SSID
  const String ssidStr = WiFi.SSID();
  const char* ssid = ssidStr.c_str();
  const char* serverURL = nullptr;
  if (strcmp(ssid, SSID1) == 0)      serverURL = SERVER_URL1;
  else if (strcmp(ssid, SSID2) == 0) serverURL = SERVER_URL2;
  else                               Serial.print(F("[ERROR] Unknown network - check SSID config!\n"));

  if (WiFi.status() == WL_CONNECTED) {
    netBusy = true;

    if (!serverURL) {
      Serial.println(F("[ERROR] No server URL configured for this SSID."));
      netBusy = false;
      return;
    }

    HTTPClient http;
    if (!http.begin(serverURL)) {
      Serial.println(F("[ERROR] http.begin failed"));
      netBusy = false;
      return;
    }

    http.setTimeout(10000);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    char jsonData[256];

    time_t nowTime = time(nullptr);
    char isoTime[25];
    strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%SZ", gmtime(&nowTime));

    doc["timestamp"] = isoTime;
    doc["temp_f"]    = tempF;
    doc["humidity"]  = humidity;
    doc["lux"]       = lux;
    doc["moisture"]  = moisture;
    //doc["raw_white"] = raw_white;
    //doc["raw_als"]   = raw_als;

    size_t len = serializeJson(doc, jsonData, sizeof(jsonData));
    int httpResponse = http.POST((uint8_t*)jsonData, len);
    if (httpResponse > 0) {
      Serial.printf("[HTTP] POST Success: %d\n", httpResponse);
    } else {
      Serial.printf("[HTTP] POST failed: %s\n", http.errorToString(httpResponse).c_str());
    }
    http.end();
    netBusy = false;
  } else {
    Serial.println("[ERROR] - SensorTask: wifi not connected.");
  }

  // ~6s OTA window before sleep
  for (int i = 0; i < 6000; i += 100) {
    if (otaInProgress) {
      Serial.println("OTA IN PROGRESS");
      return; // stay awake to finish OTA
    }
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  esp_task_wdt_reset();
  goToSleep();
}

void otaTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  for (;;) {
    ArduinoOTA.handle();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    esp_task_wdt_reset();
  }
}

// -------- Setup / Loop --------
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  analogSetPinAttenuation(MOISTURE_PIN, ADC_11db);
  connectWiFi_OTA();
  if (time(nullptr) < 10000 || (bootCount % 12 == 0)) {
    syncTime();
  }
  initNVS();
  loadFromNVS(sleep_sec, cfg_version);     // hydrates globals from NVS or defaults
  fetchConfig();                           // pulls sleep (sec) + cfg_version from server
  initSensors();
  esp_task_wdt_init(60, true);
  ++bootCount;
  Serial.printf("[SETUP] Boot count: %d\n", bootCount);


  xTaskCreatePinnedToCore(
    sensorTask, "Sensor Task",
    4096, NULL, 1, NULL, 1
  );
  xTaskCreatePinnedToCore(
    otaTask, "OTA Task",
    2048, NULL, 1, NULL, 0
  );
}

void loop() {
  // nothing
}
