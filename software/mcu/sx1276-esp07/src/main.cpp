#include "Arduino.h"
#include "ESP8266WiFi.h"

ADC_MODE(ADC_VCC);

bool configured = false;

String g_conf_ap = "S_GW_0123"; // TODO: generate from MAC address
String g_ssid = "fauxpost";     // TODO: read from EEPROM or SPIFFS

void diag_info()
{
  WiFi.printDiag(Serial); os_printf("\n");
  byte mac[6];
  WiFi.macAddress(mac);
  os_printf("MAC1: %02x:%02x:%02x:%02x:%02x:%02x ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  WiFi.softAPmacAddress(mac);
  os_printf("MAC2: %02x:%02x:%02x:%02x:%02x:%02x ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  os_printf("\n\r");
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("ESP07-inAir9 Gateway");

  // Mode of operation
  // (a) If unconfigured, start as an AP so the user can connect
  //     using a mobile device & configure the SSID
  // (b) When configured, connect to the AP SSID

  if (!configured) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(g_conf_ap.c_str());
  } else {
    WiFi.begin(g_ssid.c_str());
  }
}

int loop_counter = 0;

void loop()
{
  bool firstDiag = loop_counter == 1;
  if (firstDiag) {
    diag_info();
  }
  
  if (!configured) {
    // waiting for configuration...
    if (firstDiag) {
      Serial.println("Waiting for user configuration.");
    }
  } else {
  }
  
  delay(5000);
  loop_counter++;
}
