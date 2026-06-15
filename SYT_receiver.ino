#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0,U8X8_PIN_NONE,22,21);

typedef struct struct_message {
  float raumTemp;
  float raumHum;
  float wetterTemp;
  float wetterHum;
  char ort[32];
} struct_message;

struct_message daten;

// KORREKTUR: Alte Callback-Signatur ohne 'esp_now_recv_info_t'
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&daten, incomingData, sizeof(daten));
  Serial.println("Daten empfangen");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  delay(100);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Fehler");
    return;
  }

  // Jetzt passt die Funktion onReceive zur Erwartung des Board-Cores
  esp_now_register_recv_cb(onReceive);

  Wire.begin(21, 22);

  display.begin();
  display.enableUTF8Print();

  Serial.println("Receiver bereit");
}

void loop() {
 display.clearBuffer();					// Internen Puffer leeren
  display.setFont(u8g2_font_6x10_tf);	// Kleine Schrift wählen

  // Raumwerte
  display.drawStr(0, 10, "Raumwerte:");
  display.setCursor(0, 22);
  display.print(daten.raumTemp); display.print(" C");
  display.setCursor(0, 32);
  display.print(daten.raumHum); display.print(" %");

  // Wetter
  display.drawStr(0, 48, daten.ort);
  display.setCursor(0, 58);
  display.print(daten.wetterTemp); display.print(" C / ");
  display.print(daten.wetterHum); display.print(" %");

  display.sendBuffer();					// Alles auf das Display übertragen
  delay(1000);
}
