#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_HDC1000.h>
#include <BH1750.h>

#define MOISTURE_PIN 34

// Struct for multiple known WiFi networks
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

// List of known WiFi networks. Uncomment SSID3 and SSID4 if you need more networks.
WiFiCredentials knownNetworks[] = {
  { "SSID1", "PASSWORD" }, // WiFi credentials go here
  { "SSID2", "PASSWORD" } // WiFi credentials go here
//,{ "SSID3", "PASSWORD"}
//,{"SSID4", "PASSWORD"}  
};

const int numNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

Adafruit_HDC1000 hdc;
BH1750 lightMeter;

void connectWiFi() {
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
        Serial.printf("  Connecting to %s...\n", knownNetworks[i].ssid);
        WiFi.begin(knownNetworks[i].ssid, knownNetworks[i].password);

        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
          delay(500);
          Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\nConnected!");
          Serial.println(WiFi.localIP());
          return;
        } else {
          Serial.println("\nFailed to connect!");
        }
      }
    }
  }

  Serial.println("No known networks found!");
}

int readMoisturePercent() {
  int rawVal = analogRead(MOISTURE_PIN);
  int moisturePercent = map(rawVal, 1960, 3100, 0, 100); // These values are calibrated to my VH400 moisture sensor, you may need to change these to suit your sensor.
  return constrain(moisturePercent, 0, 100);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL

  connectWiFi();

  if (!hdc.begin()) {
    Serial.println("❌ HDC1080 not initialized!");
  }

  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("❌ BH1750 not initialized!");
  } else {
    lightMeter.setMTreg(40);
  }
}

void loop() {
  // Sensor readings
  float tempC = hdc.readTemperature();
  float tempF = (tempC * 9.0 / 5.0) + 32.0;
  float humidity = hdc.readHumidity();
  float lux = lightMeter.readLightLevel();
  float moisture = readMoisturePercent();

  // Determine server URL based on WiFi SSID
  String serverURL;
  if (WiFi.SSID() == "SSID1") {
    serverURL = "http://<IP_ADDRESS>:8000/api/sensor";  // EDIT THIS IP ADDRESS
  } else if (WiFi.SSID() == "SSID2") {
    serverURL = "http://<IP_ADDRESS>:8000/api/sensor"; // EDIT THIS IP ADDRESS
    // else if (WiFi.SSID() == "SSID3") {           // UNCOMMENT THESE IF USING 3+ SSIDs
    // serverURL = "http://<IP_ADDRESS>:8000/api/sensor"
    // else if (WiFi.SSID() == "SSID4") {
    // serverURL = "http://<IP_ADDRESS>:8000/api/sensor"
    }
    else {
    Serial.println("Unknown network — no server URL set!");
    return;
  }

  // Send data if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL.c_str());
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    doc["temp_f"] = tempF;
    doc["humidity"] = humidity;
    doc["lux"] = lux;
    doc["moisture"] = moisture;

    String jsonData;
    serializeJson(doc, jsonData);

    Serial.println("-------------------------------------");
    int httpResponse = http.POST(jsonData);
    Serial.print(F("HTTP Response Code: "));
    Serial.println(httpResponse);
    Serial.println("Response body: " + http.getString());
    Serial.println("-------------------------------------");

    http.end();
  } else {
    Serial.println(F("WiFi Disconnected!"));
  }

  // Print sensor values to serial monitor
  Serial.printf("Temp C: %.2f, Temp F: %.2f, Humidity: %.2f, Lux: %.2f, Moisture: %.2f\n",
                tempC, tempF, humidity, lux, moisture);

  delay(300000);  // 5-minute delay
}
