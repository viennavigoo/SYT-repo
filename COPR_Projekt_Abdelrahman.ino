#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WebServer.h>
#include <esp_now.h>
#include <Wire.h>
#include <esp_wifi.h>

// WLAN-Daten
const char* ssid1 = "abdel";
const char* password1 = "hexeflugzeug";

const char* ssid2 = "UPCBAF4A58";
const char* password2 = "Sa7xthxdwmQc";

// Webserver
WebServer server(80);

// JSON Daten
String ort;
float temperatur;
float druck;

// Sensoren
OneWire oneWire(12);
DallasTemperature sensors(&oneWire);
DHT dht(4, DHT11);

// ================= ESP NOW =================

uint8_t broadcastAddress[] = {0x00, 0x70, 0x07, 0x26, 0xB7, 0xAC};

typedef struct struct_message {
  float raumTemp;
  float raumHum;
  float wetterTemp;
  float wetterHum;
  char ort[32];
} struct_message;

struct_message daten;


void connectToWiFi() {

  Serial.println("Verbinde mit WLAN 1...");
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid1, password1);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 3) {
    delay(10000);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("\nWLAN 1 fehlgeschlagen.");
    Serial.println("Verbinde mit WLAN 2...");

    WiFi.begin(ssid2, password2);

    timeout = 0;

    while (WiFi.status() != WL_CONNECTED && timeout < 3) {
      Serial.print(".");
      delay(3000);
      timeout++;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nVerbunden!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nKeine WLAN-Verbindung möglich.");
  }
}


void jsonFilter() {

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("https://cdn3.techweb.at/api/weather/at/data?province=wien&format=json");

  int codeWebsite = http.GET();

  if (codeWebsite == 200) {

    String payload = http.getString();

    StaticJsonDocument<8192> doc;

    if (!deserializeJson(doc, payload)) {

      for (JsonObject station : doc["weather_data"].as<JsonArray>()) {

        const char* tempOrt = station["location"];

        if (strcmp(tempOrt, "Wien Donaufeld") == 0) {

          ort = String(tempOrt);
          temperatur = station["temperature"];
          druck = station["humidity"];
          break;
        }
      }
    }
  }

  http.end();
}



void html() {

  sensors.requestTemperatures();

  float tempC = dht.readTemperature();
  float hum = dht.readHumidity();

  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='UTF-8'><meta http-equiv='refresh' content='5'></head>";

  html += "<body><h1>ESP32 Wetterstation</h1>";

  html += "<h2>Raumwerte</h2>";
  html += "Temp: " + String(tempC) + " °C<br>";
  html += "Hum: " + String(hum) + " %<br>";

  html += "<h2>Wien Donaufeld</h2>";
  html += "Ort: " + ort + "<br>";
  html += "Temp: " + String(temperatur) + " °C<br>";
  html += "Hum: " + String(druck) + " %<br>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}



void onSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-NOW FAIL");
  }
}



void setup() {

  Wire.begin();
  Serial.begin(115200);

  sensors.begin();
  dht.begin();

  connectToWiFi();

  // Webserver
  server.on("/", html);
  server.begin();

  Serial.println("Webserver gestartet");


WiFi.mode(WIFI_STA); 
esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Fehler");
    return;
}

esp_now_register_send_cb(onSent);

esp_now_peer_info_t peerInfo = {};
memcpy(peerInfo.peer_addr, broadcastAddress, 6);


peerInfo.ifidx = WIFI_IF_STA; 
peerInfo.channel = 0; 
peerInfo.encrypt = false;

if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer Fehler");
    return;
}
}


void loop() {

  server.handleClient();

  sensors.requestTemperatures();
  
  float tempC = sensors.getTempCByIndex(0);
  float hum = dht.readHumidity();
  float tempCDHT = dht.readTemperature();

  jsonFilter();
  if(tempC == 127.00 || tempC == -127.00){
    daten.raumTemp = tempCDHT;
  }
  else{
    daten.raumTemp = tempC;
  }
  
  daten.raumHum = hum;

  daten.wetterTemp = temperatur;
  daten.wetterHum = druck;

  strcpy(daten.ort, ort.c_str());

  esp_now_send(broadcastAddress, (uint8_t*)&daten, sizeof(daten));
  Serial.println("Daten gesendet");

  delay(2000);
}