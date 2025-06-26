#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Wire.h>
#include <Time.h>
#include <Adafruit_HDC1000.h>
#include "Adafruit_VEML7700.h"
#include "esp_task_wdt.h"

#if defined (BOARD_ESP32)
  #define SDA 21
  #define SCL 22
  #define MOISTURE_PIN 35
#elif defined (BOARD_ESP32S3)
  #define SDA 8
  #define SCL 9
  #define MOISTURE_PIN 4
#endif

#define uS_TO_S_FACTOR 1000000ULL


RTC_DATA_ATTR int sleepDuration = 30; // Seconds (default 600s = 10 min)
RTC_DATA_ATTR int bootCount = 0;

// Hardcoded Credentials
const char* SSID1 = "<SSID1>"; // CHANGE ME
const char* PASSWORD1 = "<PASSWORD1>"; // CHANGE ME
const char* SSID2 = "<SSID2>"; // CHANGE ME
const char* PASSWORD2 = "<PASSWORD2"; // CHANGE ME
const char* CONFIG_URL1 = "http://<IP_ADDRESS1>:8000/api/config"; // UPDATE WITH SERVER IP
const char* CONFIG_URL2 = "http://<IP_ADDRESS2>:8000/api/config"; // UPDATE WITH SERVER IP
const char* SERVER_URL1 = "http://<IP_ADDRESS1>:8000/api/sensor"; // UPDATE WITH SERVER IP
const char* SERVER_URL2 = "http://<IP_ADDRESS2>:8000/api/sensor"; // UPDATE WITH SERVER IP

// Config file variables
String config_ssid = "";
String config_password = "";
String mqtt_broker = "";
int mqtt_port = 1883;

// Sensor object creation
Adafruit_HDC1000 hdc;
Adafruit_VEML7700 veml = Adafruit_VEML7700();

struct WiFiCredentials {
  const char* ssid;
  const char* password;
}; WiFiCredentials knownNetworks[] = {
  { SSID1, PASSWORD1 }, { SSID2, PASSWORD2 } 
}; const int numNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

void goToSleep() {
  Serial.println("Going to sleep...");
  esp_task_wdt_reset();
  delay(100);
  esp_sleep_enable_timer_wakeup(sleepDuration * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}


bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print(F("Scanning for networks...\n"));
  int networksFound = WiFi.scanNetworks();
  if (networksFound == 0) {
    Serial.print(F("No networks found!\n"));
    return false;
  }
  for (int i = 0; i < numNetworks; i++) {
    for (int j = 0; j < networksFound; j++) {
      if (WiFi.SSID(j) == knownNetworks[i].ssid) {
        Serial.printf("Connecting to %s...", knownNetworks[i].ssid);
        WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
          delay(500);
          Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
          Serial.printf("\nDevice IP address: %s\n", WiFi.localIP().toString().c_str());
          Serial.printf("WiFi Signal Strength: %d dBm\n",WiFi.RSSI());
          return true;  
        } else {
          Serial.print(F("Failed to connect to wifi.\n"));
        }
      }
    }
  }
  Serial.print(F("No known networks found, double check your WiFi credentials!\n"));
  return false;
}

volatile bool otaInProgress = false;

void setupOTA() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("garden-sensor");
  ArduinoOTA.onStart([]() { otaInProgress = true;Serial.println("\nStart OTA Update"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA update complete."); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("\nProgress:  %u%%\r", (progress / (total / 100 )));
  });
  ArduinoOTA.begin();
  delay(1000);
  Serial.print(F("OTA Available\n"));
}

void connectWiFi_OTA() {
  if (connectWiFi()) {
    setupOTA();
  } else {
    Serial.print(F("WiFi connection failed. OTA not available.\n"));
  }
}

bool connectConfig(const char* ssid, const char* password, unsigned long timeoutMs = 10000) {
  if (ssid == nullptr || password == nullptr || strlen(ssid) == 0 || strlen(password) == 0) {
    Serial.print(F("No wifi credentials in config\n"));
    return false;
  }
WiFi.begin(ssid, password);
Serial.printf("Connecting to SSID: %s\n", ssid);
unsigned long start = millis();
while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
  delay(500);
  Serial.print(".");
}
if (WiFi.status() == WL_CONNECTED) {
  Serial.print("\nConnected! IP: ");
  Serial.println(WiFi.localIP());
  return true;
} else {
  Serial.print(F("\nFailed to connect to wifi."));
  return false;
  }
}

void syncTime() {
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print(F("Waiting for time sync..."));
  while (time(nullptr) < 100000) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(F("\nSynced!\n"));
}

void initSensors() {
  bool vemlReady = false;

  if (!hdc.begin()) {
    Serial.print(F("HDC1080 init failed.\n"));
    return;
  }
  vemlReady = veml.begin();
  delay(10);

  // Light sensor configuration
  veml.setGain(VEML7700_GAIN_1_8);
  veml.setIntegrationTime(VEML7700_IT_50MS);
  veml.interruptEnable(false);
  veml.setPersistence(true);
  veml.powerSaveEnable(false);
  veml.setPowerSaveMode(VEML7700_POWERSAVE_MODE4);
  //veml.setLowThreshold(10000);
  //veml.setHighThreshold(80000);
}

int readMoisturePercent() {
  int minVal = 1200;    // Lower-threshold
  int maxVal = 4000;    // Upper-threshold
  int rawVal = analogRead(MOISTURE_PIN);
  int moisturePercent = map(rawVal, minVal, maxVal, 0, 100); // Calibrated to VH400 sensor, you may need to swap minVal/maxVal for yours
  moisturePercent = constrain(moisturePercent, 0, 150); // Calibrated to 150% to monitor overwatering
  // DEBUG LINES, CAN BE REMOVED //
  Serial.printf("[DEBUG] Raw moisture:  %d\n", rawVal);
  return moisturePercent;
}

void sensorTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  esp_task_wdt_reset();

  float tempC = hdc.readTemperature();
  float tempF = (tempC * 9.0 / 5.0) + 32.0;
  float humidity = hdc.readHumidity();
  float lux = veml.readLux();
  float moisture = readMoisturePercent();
  float raw_als = veml.readALS();
  float raw_white = veml.readWhite();

  Serial.printf("[INFO] Temp(F): %.2f, Humidity: %.2f, Lux: %.2f, Moisture: %.2f\n", tempF, humidity, lux, moisture);
  
  // DEBUG LINES, CAN BE REMOVED //
  Serial.printf("[DEBUG] ALS: %.2f, WHITE: %.2f\n", raw_als, raw_white);
  Serial.printf("[DEBUG] RSSI: %d dBm\n", WiFi.RSSI());
  Serial.printf("[DEBUG] Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("[DEBUG] Integration time: %d ms\n", veml.getIntegrationTimeValue());
  Serial.printf("[DEBUG] Gain Multiplier: x%.3f\n", veml.getGainValue());

  String ssidStr = WiFi.SSID();
  const char* ssid = ssidStr.c_str();
  const char* serverURL = nullptr;
  if (strcmp(ssid,SSID1) == 0) {
      serverURL = SERVER_URL1;
  } else if (strcmp(ssid, SSID2) == 0) {
    serverURL = SERVER_URL2;
  } else {
      Serial.print(F("Unknown network - check SSID config.\n"));
  }
  
  if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);
      http.setTimeout(10000);  
      http.addHeader("Content-Type", "application/json");
      
      StaticJsonDocument<200> doc;
      char jsonData[512];
      time_t nowTime = time(nullptr);
      char isoTime[30];
      strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%SZ", gmtime(&nowTime));
      
      doc["timestamp"] = isoTime;
      doc["temp_f"] = tempF;
      doc["humidity"] = humidity;
      doc["lux"] = lux;
      doc["moisture"] = moisture;
      doc["raw_white"] = raw_white;
      doc["raw_als"] = raw_als;
      //doc["integration_time"] = integration_time;
      
      serializeJson(doc, jsonData, sizeof(jsonData));
      int httpResponse = http.POST((uint8_t*)jsonData, strlen(jsonData));
      
      if (httpResponse > 0) {
          Serial.printf("POST Success: %d\n", httpResponse);
      } else {
          Serial.printf("POST failed: %s\n", http.errorToString(httpResponse).c_str());
      }
      
      http.end();
      
  } else {
    Serial.println("[ERROR] - SensorTask: WiFi Not connected.");
  }
    for (int i = 0; i < 6000; i+= 100) {
    if (otaInProgress) {
      Serial.println("OTA IN PROGRESS");
      return;
    }
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  esp_task_wdt_reset(); // feed the watchdog
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

void wifiMonitorTask(void *pvParameters) {
for (;;) {
  esp_task_wdt_add(NULL);
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    delay(1000);
    Serial.print(F("WiFi lost. Attempting reconnect...\n"));
    connectWiFi_OTA();
  }
  esp_task_wdt_reset();
  vTaskDelay(15000 / portTICK_PERIOD_MS);
}
}

void fetchConfig() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(CONFIG_URL1);
  int httpCode = http.GET();
  Serial.print(F("Fetching configuration..."));
  if (httpCode == 200) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, http.getStream());
    if (!error) {
      if (doc.containsKey("polling_rate")) {
        sleepDuration = doc["polling_rate"];
        Serial.printf("Updated sleepDuration to: %d seconds.\n", sleepDuration);
      }
      else {
        Serial.print(F("Failed to parse config.\n"));
      }
    } else { 
      Serial.printf("Failed to fetch config. HTTP Code: %d\n", httpCode);
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  analogSetPinAttenuation(MOISTURE_PIN, ADC_11db);
  connectWiFi_OTA();
  syncTime();
  fetchConfig();
  initSensors();
  esp_task_wdt_init(30, true);
  ++bootCount;
  Serial.printf("Boot count: %d\n", bootCount);  
  //esp_task_wdt_reset();

  xTaskCreatePinnedToCore(
  sensorTask,
  "Sensor Task",
  4096,
  NULL,
  1,
  NULL,
  1  // Runs on core 1
  );

  xTaskCreatePinnedToCore(
    otaTask,
    "OTA Task",
    2048,
    NULL,
    1,
    NULL,
    0 // Runs on core 0
  );

  xTaskCreatePinnedToCore(
    wifiMonitorTask,
    "WiFi Monitor Task",
    2048,
    NULL,
    1,
    NULL,
    0 // Runs on core 0
  );
}

void loop() {
  // nothing here
}