#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_HDC1000.h>
#include <ClosedCube_OPT3001.h>

#define MOISTURE_PIN 35
#define uS_TO_S_FACTOR 1000000ULL
#define SLEEP_DURATION 600  // in seconds (e.g., 600s = 10 minutes)

RTC_DATA_ATTR int bootCount = 0;

Adafruit_HDC1000 hdc;
ClosedCube_OPT3001 opt3001;

struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

WiFiCredentials knownNetworks[] = {
  { "SSID1", "PASSWORD1" },
  { "SSID2", "PASSWORD2" }
};

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();
  for (int i = 0; i < sizeof(knownNetworks) / sizeof(knownNetworks[0]); i++) {
    for (int j = 0; j < n; j++) {
      if (WiFi.SSID(j) == knownNetworks[i].ssid) {
        WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
          delay(500);
        }
        if (WiFi.status() == WL_CONNECTED) return;
      }
    }
  }
}

int readMoisturePercent() {
  int raw = analogRead(MOISTURE_PIN);
  float percent = 0;
  if (raw <= 1000) percent = map(raw, 0, 1000, 0, 20);
  else if (raw <= 2000) percent = map(raw, 1000, 2000, 20, 50);
  else if (raw <= 2600) percent = map(raw, 2000, 2600, 50, 85);
  else if (raw <= 3000) percent = map(raw, 2600, 3000, 85, 100);
  else if (raw <= 3278) percent = map(raw, 3000, 3278, 100, 120);
  else percent = 125;
  return round(constrain(percent, 0, 125));
}

void setup() {
  Serial.begin(115200);
  delay(200);

  ++bootCount;
  Serial.printf("Boot #%d\n", bootCount);

  analogSetPinAttenuation(MOISTURE_PIN, ADC_11db);
  Wire.begin(21, 22);

  connectWiFi();

  if (!hdc.begin()) Serial.println("HDC1080 init failed.");
  if (opt3001.begin(0x44) != NO_ERROR) Serial.println("OPT3001 init failed.");
  
  OPT3001_Config config = opt3001.readConfig();
  config.RangeNumber = B1100;
  config.ConvertionTime = B1;
  config.ModeOfConversionOperation = B11;
  opt3001.writeConfig(config);

  float tempF = (hdc.readTemperature() * 9.0 / 5.0) + 32.0;
  float humidity = hdc.readHumidity();
  float lux = opt3001.readResult().lux;
  float moisture = readMoisturePercent();

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverURL = "http://<IP_ADDRESS>:8000/api/sensor"; // adjust if needed
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["timestamp"] = time(nullptr);
    doc["temp_f"] = tempF;
    doc["humidity"] = humidity;
    doc["lux"] = lux;
    doc["moisture"] = moisture;

    String jsonStr;
    serializeJson(doc, jsonStr);
    http.POST(jsonStr);
    http.end();
  }

  Serial.println("Going to sleep now...");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // Nothing here — we never return from deep sleep
}
