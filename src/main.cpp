#include <Arduino.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "freertos/task.h"
#include "secrets.h"

const bool useExistingNetwork = USE_EXISTING_NETWORK;
const bool hideNetwork = HIDE_NETWORK;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

const char *existingSsid = EXISTING_WIFI_SSID;
const char *existingPassword = EXISTING_WIFI_PASSWORD;

const int output26 = 26;
const int activeLength = 47500;
bool restartOutput = false;
int restartDelay = activeLength; // this is in milliseconds (ms)


AsyncWebServer server(80);

void togglePin()
{
  digitalWrite(output26, HIGH);
  delay(100);
  digitalWrite(output26, LOW);
}

void restartPin() {
  while (restartOutput) {
    digitalWrite(output26, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(output26, LOW);
    vTaskDelay(pdMS_TO_TICKS(restartDelay));
  }
}

TaskHandle_t taskHandle = NULL;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting...");

  WiFi.setHostname("ESP32-SERVER");
  if (useExistingNetwork) {
    // Join WiFi network
    WiFi.mode(WIFI_STA);
    WiFi.begin(existingSsid, existingPassword);
    Serial.print("\nConnecting");

    while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
    }
    Serial.println("");
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    // Create WiFi Network
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password, 1, hideNetwork);
    Serial.println(WiFi.softAPIP());
  }


  pinMode(output26, OUTPUT);

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request)
  { 
    togglePin();
    request -> send(202); 
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonDocument response;
    if (json.containsKey("restartOutput")) {
      restartOutput = json["restartOutput"];
      response["restart"] = restartOutput;
    }
    
    if (json.containsKey("restartOutput")) {
      restartDelay = json["restartDelay"];
      
      // If completeSong is in the body and false then don't add it, otherwise always add it.
      restartDelay += activeLength * (json.containsKey("completeSong") ? json["completeSong"] : true);
      
      response["delay"] = json["restartDelay"];
      response["activeLength"] = activeLength;
      response["totalDelay"] = restartDelay;
    }

    if (json.containsKey("restartOutput") | json.containsKey("restartDelay")) {
      response["status"] = "OK";
      String res;
      serializeJson(response, res);

      request -> send(200, "application/json", res);
    }
    else {
      request -> send(400, "application/json", "{\"error\": \"No body\"}");
    }
  });
  server.addHandler(handler);

  server.begin();
}

void loop() { restartPin(); }