#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>    

#define DEFAULT_PASSWORD "password"

ADC_MODE(ADC_VCC);

static std::unique_ptr<ESP8266WebServer> webServer;

// GET http://gateway        <-- status page
// GET http://gateway/reset  <-- API call: drop wifi configuration, reset back into AP + captive portal

static String apSSID = "gw_" + String(ESP.getChipId());

void handleRoot() {
  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Gateway Status");
  page += FPSTR(HTTP_HEAD_END);
  page += "<h1>";
  page += "Gateway Status";
  page += "</h1>";
  page += FPSTR(HTTP_END);
  webServer->send(200, "text/html", page);
}

void handleReset() {
  String page = FPSTR("Reset OK.");  
  page += FPSTR(" Look for AP SSID: ");
  page += apSSID;
  webServer->send(200, "text/html", page);
  delay(500);

  Serial.println(F("USER RESET of WIFI DATA. Reboot..."));
  WiFi.disconnect(); delay(500);
  ESP.restart(); delay(500);
}

void handleNotFound() {
  webServer->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer->sendHeader("Pragma", "no-cache");
  webServer->sendHeader("Expires", "-1");
  webServer->send ( 404, "text/plain", "sorry");
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("\n\rESP07-inAir9 Gateway");
  Serial.print("ChipId="); Serial.println(ESP.getChipId());

  String defaultSSID = WiFi.SSID();
  Serial.print("Default SSID="); Serial.println(defaultSSID);

  // if the reboot was due to _power_on_ (not reset button)
  // then sleep 2 seconds and then check if there is a short over
  // GPIO (the pin normally used for programming)
  // So the user needs to reboot first, and then press it _after_ 2 seconds...
  // and continue to hold it
  Serial.print(F("RESET: ")); Serial.println(ESP.getResetReason());
  Serial.print(F("Short GPIO from _just after_ reboot for 3 seconds to reset WIFI data"));
  if (ESP.getResetInfoPtr()->reason == REASON_DEFAULT_RST) {
    // TODO: turn on LED to indicate that button needs to be pressed..
    delay(2000);
    int npress = 0;
    while (digitalRead(0) == 0 && npress < 7) {
      Serial.println(F("."));
      npress ++;
      delay(300);
    }
    if (npress >= 7) {
      Serial.println(F("USER RESET of WIFI DATA. Reboot..."));
      WiFi.disconnect(); delay(500);
      ESP.restart(); delay(500);
    }
  }

  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(true);
  if (!wifiManager.autoConnect(apSSID.c_str(), DEFAULT_PASSWORD)) {
    // With the default flow and no timeouts we should never get here
    Serial.println(F("Unable to auto-configure. Reset WIFI DATA. Reboot..."));
    // "forget" the client SSID that we try to connect to on next boot...
    // The delay() calls seem to mititgate otherwise lockup + garbage printed
    WiFi.disconnect(); delay(500);
    ESP.restart(); delay(500);
  }

  // We now get to here only when there is a valid client SSID
  // and we have managed to connect to it.
  // The captive portal web server is also now shut down.

  // Note: it would seem that setting an ad-hoc for the client SSD hangs the AP!
  
  // Launch web server.
  // Note our IP address will be pseudo-random (by DHCP) unless
  // we somehow do something like mDNS
  webServer.reset(new ESP8266WebServer(80));
  webServer->on("/", handleRoot);
  webServer->on("/reset", handleReset);
  webServer->onNotFound(handleNotFound);
  webServer->begin();
}

static int loop_counter = 0;

void loop()
{
  if (loop_counter == 0) { Serial.println(F("~=~=~=~=~=~=~=~ loop ~=~=~=~=~=~=~=~ ")); }
  webServer->handleClient();
  loop_counter++;
  delay(100);
}
