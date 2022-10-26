/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp8266-web-server-gauges/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 2
#define MOISTUREPIN 14  
#define DHTTYPE    DHT22 

DHT dht(DHTPIN, DHTTYPE);



// Replace with your network credentials
const char* ssid = "Barribal_WiFi";
const char* password = "Welcome@123";

const char *ssid_ap = "NodeMCU";
const char *password_ap = "1234567890";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 5000;

float humidity;
float temperature;
int moisture;


// Get Sensor Readings and return JSON object
String getSensorReadings(){
  readings["temperature"] = String(temperature);
  readings["humidity"] =  String(humidity);
  readings["moisture"] = String(moisture);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {

  //WiFi Mode
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // Serial.print("Connecting to WiFi ..");

  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print('.');
  //   delay(1000);
  // }
  // Serial.println(WiFi.localIP());


  //AP MODE
  WiFi.softAP(ssid_ap, password_ap);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);



}

void runDHT(){
  float h = dht.readHumidity();
  humidity = h;
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  temperature =t;
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(h);
  Serial.println(" *F");
}

void runMoistureSensor(){

  moisture = analogRead(MOISTUREPIN);
  moisture = map(moisture, 1023, 0, 0, 100);
  Serial.print("Soil Moisture: ");
  Serial.println(moisture);
  Serial.println(" %");
  }

 

void setup() {
  Serial.begin(115200);
  dht.begin();
  initWiFi();
  initFS();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");
  
  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 30 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }
  runDHT();
  runMoistureSensor();
}
