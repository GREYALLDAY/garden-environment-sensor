#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Wire.h>
#include <Time.h>
#include <Adafruit_HDC1000.h>
#include <ClosedCube_OPT3001.h>

#define MOISTURE_PIN 35

// WiFi network setup
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};
// Update credentials here
WiFiCredentials knownNetworks[] = {
  { "SSID1", "PASSWORD1" },  // CHANGE SSID1 and PASSWORD1
  { "SSID2", "PASSWORD2" }  // CHANGE SSID2 and PASSWORD2
};
const int numNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

// Sensor object creation
Adafruit_HDC1000 hdc;
ClosedCube_OPT3001 opt3001;

// Convert time from UTC to CST
void syncTime() {
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for time sync...");
  while (time(nullptr) < 100000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nSynced!");
}

void connectWiFi_OTA() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Scanning for networks...");
  int networksFound = WiFi.scanNetworks();
  if (networksFound == 0) {
    Serial.println("No networks found!");
    return;
  }
  for (int i = 0; i < numNetworks; i++) {
    for (int j = 0; j < networksFound; j++) {
      if (WiFi.SSID(j) == knownNetworks[i].ssid) {
        Serial.printf("Connecting to %s...\n", knownNetworks[i].ssid);
        WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
          delay(500);
          Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
          Serial.printf("\nDevice IP address: %s\n", WiFi.localIP().toString().c_str());
          ArduinoOTA.setPort(3232);
          ArduinoOTA.setHostname("garden-sensor");
          ArduinoOTA.onStart([]() {
            Serial.println("Start OTA Update");
          });
          ArduinoOTA.onEnd([]() {
            Serial.println("\nOTA update complete.");
          });
          ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("\nProgress: %u%%\r", (progress / (total / 100 )));
          });
          ArduinoOTA.begin();
          delay(1000);
          Serial.println("OTA Available");
          return;
        } else {
          Serial.println("Failed to connect!");
        }
      }
    }
  }
  Serial.println("No known networks found, double check your WiFi credentials!");
}



int readMoisturePercent() {
  int minVal = 1200;    // Lower-end
  int maxVal = 4000;    // Upper-end
 
  int rawVal = analogRead(MOISTURE_PIN);
  int moisturePercent = map(rawVal, minVal, maxVal, 0, 100); // Calibrated to VH400 sensor, you may need to swap minVal/maxVal for yours
  moisturePercent = constrain(moisturePercent, 0, 150); // Calibrated to 150% to monitor overwatering
  Serial.print("[DEBUG] Raw moisture: "); // DEBUG, CAN BE REMOVED
  Serial.println(rawVal); // DEBUG, CAN BE REMOVED
  Serial.print("[DEBUG] Moisture Percentage: "); // DEBUG, CAN BE REMOVED
  Serial.println(moisturePercent); // DEBUG, CAN BE REMOVED
  return moisturePercent;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL
  analogSetPinAttenuation(MOISTURE_PIN, ADC_11db);
  connectWiFi_OTA();
  syncTime();

  if (!hdc.begin()) {
    Serial.println("HDC1080 init failed.");
  }
  if (opt3001.begin(0x44) != NO_ERROR) {
    Serial.println("OPT3001 init failed.");
    while (1);
  }

  // Configure light sensor
  OPT3001_Config config = opt3001.readConfig();
  config.RangeNumber = B1100; // auto-range
  config.ConvertionTime = B1; // 800ms, higher accuracy
  config.ModeOfConversionOperation = B01; // one-shot reading, saves battery life
  opt3001.writeConfig(config);
}


const unsigned long updateInterval = 300000; // UPDATE INTERVAL (CHANGE ME AS NEEDED)
unsigned long lastUpdate = -updateInterval;

void loop() {
  ArduinoOTA.handle();

  unsigned long now = millis(); // non-blocking timer to handle reading intervals
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;

    float tempC = hdc.readTemperature();
    float tempF = (tempC * 9.0 / 5.0) + 32.0;
    float humidity = hdc.readHumidity();
    float lux = -1;
  
    OPT3001 result = opt3001.readResult();
    if (result.error == NO_ERROR) {
      lux = result.lux;
    } else {
      Serial.print("light sensor read error: ");
      Serial.println(result.error);
    }

    float moisture = readMoisturePercent();

    Serial.printf("Temp(F): %.2f, Humidity: %.2f, Lux: %.2f, Moisture: %.2f\n", tempF, humidity, lux, moisture);

    String serverURL;
    if (WiFi.SSID() == "SSID1") { // CHANGE SSID1
      serverURL = "http://<IP_ADDRESS>:8000/api/sensor"; // CHANGE <IP_ADDRESS> 
    } else if (WiFi.SSID() == "SSID2") { // CHANGE SSID2
      serverURL = "http://<IP_ADDRESS>:8000/api/sensor"; // CHANGE <IP_ADDRESS>
    } else {
      Serial.println("Unknown network — no server URL set.");
      return;
    }

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL.c_str());
      http.addHeader("Content-Type", "application/json");

      StaticJsonDocument<200> doc;
      time_t nowTime = time(nullptr);
      char isoTime[30];
      strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%SZ", gmtime(&nowTime));
      doc["timestamp"] = isoTime;
      doc["temp_f"] = tempF;
      doc["humidity"] = humidity;
      doc["lux"] = lux;
      doc["moisture"] = moisture;

      String jsonData;
      serializeJson(doc, jsonData);
      int httpResponse = http.POST(jsonData);
      String response = http.getString();
      http.end();
    } else {
      Serial.println("Wifi disconnected");
    }
  }
}
