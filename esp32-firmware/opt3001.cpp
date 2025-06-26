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
#define SDA 21
#define SCL 22

// Hardcoded Credentials
const char* SSID1 = "<SSID1>"; // CHANGE ME
const char* PASSWORD1 = "<PASSWORD1>"; // CHANGE ME
const char* SSID2 = "<SSID2>"; // CHANGE ME
const char* PASSWORD2 = "<PASSWORD2"; // CHANGE ME
const char* CONFIG_URL1 = "http://<IP_ADDRESS1>:8000/api/config"; // UPDATE WITH SERVER IP
const char* CONFIG_URL2 = "http://<IP_ADDRESS2>:8000/api/config"; // UPDATE WITH SERVER IP
const char* SERVER_URL1 = "http://<IP_ADDRESS1>:8000/api/sensor"; // UPDATE WITH SERVER IP
const char* SERVER_URL2 = "http://<IP_ADDRESS2>:8000/api/sensor"; // UPDATE WITH SERVER IP

// Sensor polling interval
unsigned long pollingInterval = 10000;
unsigned long lastUpdate = -pollingInterval;

// Config file variables
String config_ssid = "";
String config_password = "";
String mqtt_broker = "";
int mqtt_port = 1883;

// Sensor object creation
Adafruit_HDC1000 hdc;
ClosedCube_OPT3001 opt3001;

struct WiFiCredentials {
  const char* ssid;
  const char* password;
}; WiFiCredentials knownNetworks[] = {
  { SSID1, PASSWORD1 }, { SSID2, PASSWORD2 } 
}; const int numNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

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
        Serial.printf("Connecting to %s...\n", knownNetworks[i].ssid);
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

void setupOTA() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("garden-sensor");
  ArduinoOTA.onStart([]() { Serial.println("\nStart OTA Update"); });
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

bool fetchConfig() {
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(CONFIG_URL1);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print(F("Failed to parse config file\n"));
      return false;
    }
  if (doc.containsKey("polling_rate")) {
    pollingInterval = doc["polling_rate"];
    Serial.print(F("Polling interval set to: "));
    Serial.println(pollingInterval);
  }
  if (doc.containsKey("ssid") && doc.containsKey("password")) {
    config_ssid = doc["ssid"].as<String>();
    config_password = doc["password"].as<String>();
    Serial.printf("Configured SSID: %s\n", config_ssid.c_str());
  }
  if (doc.containsKey("mqtt_broker")) {
    mqtt_broker = doc["mqtt_broker"].as<String>();
    Serial.printf("Configured MQTT broker: %s\n", mqtt_broker.c_str());
  }
  if (doc.containsKey("mqtt_port")) {
    mqtt_port = doc["mqtt_port"];
    Serial.printf("Configured MQTT port: %d\n", mqtt_port);
  } 
  http.end();
  return true;
  
} else {
    Serial.printf("Failed to fetch config, code: %d\n", httpCode);
    http.end();
    return false;
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
  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
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
  if (!hdc.begin()) {
    Serial.print(F("HDC1080 init failed.\n"));
    return;
  }
  if (opt3001.begin(0x44) != NO_ERROR) {
    Serial.print(F("OPT3001 init failed.\n"));
    return;
  }
    // light sensor config
  OPT3001_Config config = opt3001.readConfig();
  config.RangeNumber = B1100; // auto-range
  config.ConvertionTime = B1; // 800ms, higher accuracy
  config.ModeOfConversionOperation = B01; // one-shot reading, saves battery life
  opt3001.writeConfig(config);

}

int readMoisturePercent() {
  int minVal = 1200;    // Lower-end
  int maxVal = 4000;    // Upper-end
  int rawVal = analogRead(MOISTURE_PIN);
  int moisturePercent = map(rawVal, minVal, maxVal, 0, 100); // Calibrated to VH400 sensor, you may need to swap minVal/maxVal for yours
  moisturePercent = constrain(moisturePercent, 0, 150); // Calibrated to 150% to monitor overwatering
  Serial.printf("[DEBUG] Raw moisture:  %d\n", rawVal); // DEBUG, CAN BE REMOVED
  Serial.printf("[DEBUG] Moisture Percentage: %d\n", moisturePercent); // DEBUG, CAN BE REMOVED
  return moisturePercent;
}

void sensorTask(void *pvParameters) {
    for (;;) {
        unsigned long now = millis();
        if (now - lastUpdate >= pollingInterval) {
            lastUpdate = now;


        float tempC = hdc.readTemperature();
        float tempF = (tempC * 9.0 / 5.0) + 32.0;
        float humidity = hdc.readHumidity();
        float lux = -1;

        OPT3001 result = opt3001.readResult();
        if (result.error == NO_ERROR) {
            lux = result.lux;
        } else {
            Serial.print(F("light sensor read error: "));
            Serial.println(result.error);
        }

        float moisture = readMoisturePercent();
        Serial.printf("Temp(F): %.2f, Humidity: %.2f, Lux: %.2f, Moisture: %.2f\n", tempF, humidity, lux, moisture);

        String serverURL;
        if (WiFi.SSID() == SSID1) {
            serverURL = SERVER_URL1;
        } else if (WiFi.SSID() == SSID2) {
          serverURL = SERVER_URL2;
        } else {
            Serial.print(F("Unknown network - check SSID config.\n"));
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        } 
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
        if (httpResponse > 0) {
            Serial.printf("POST Success: %d\n", httpResponse);
        } else {
            Serial.printf("POST failed: %s\n", http.errorToString(httpResponse).c_str());
        }
        http.end();
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    }

}

void otaTask(void *pvParameters) {
  for (;;) {
    ArduinoOTA.handle();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void wifiMonitorTask(void *pvParameters) {
for (;;) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("WiFi lost. Attempting reconnect...\n"));
    connectWiFi_OTA();
  }
  vTaskDelay(15000 / portTICK_PERIOD_MS);
}
}

void configTask(void *pvParameters) {
  for (;;) {
    Serial.print(F("Fetching configuration...\n"));

    HTTPClient http;
    http.setTimeout(10000);
    http.begin(CONFIG_URL1);

    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("Config JSON parse error.\n"));
      } else {
        if (doc.containsKey("polling_rate")) {
          pollingInterval = doc["polling_rate"];
          Serial.printf("Updated polling interval to: %lu ms\n", pollingInterval); 
        }

      // expand here later

      }
    } else {
      Serial.printf("Config fetch failed. HTTP code: %d\n", httpCode);
    }


    http.end();
    vTaskDelay(300000 / portTICK_PERIOD_MS); // 5 minute interval
  }
}
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  analogSetPinAttenuation(MOISTURE_PIN, ADC_11db);
  connectWiFi_OTA();
  syncTime();
  initSensors();  
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
  BaseType_t result = xTaskCreatePinnedToCore(
    configTask,
    "Configuration Task",
    3072,
    NULL,
    1,
    NULL,
    0
  );
  if (result != pdPASS) {
    Serial.print(F("Failed to create configTask"));
  }
}



void loop() {
  // nothing here
}