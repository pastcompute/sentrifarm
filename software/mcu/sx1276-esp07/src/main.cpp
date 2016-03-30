/*
  Copyright (c) 2016 Andrew McDonnell <bugs@andrewmcdonnell.net>

  This file is part of SentriFarm Radio Relay.

  SentriFarm Radio Relay is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SentriFarm Radio Relay is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SentriFarm Radio Relay.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>    

#include "mcu.h"
#include "globals.h"
#include "misc.h"
#include "web.h"

#include "elapsedMillis.h"
#include "sx1276.h"

using sentrifarm::WebServer;
using sentrifarm::CheckForUserResetWifiOnBoot;
using sentrifarm::DumpRadioRegisters;
using sentrifarm::LEDFlashRadioNotFoundPattern;

ADC_MODE(ADC_VCC);

static SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);

static SX1276Radio radio(PIN_SX1276_CS, spiSettings);

static WebServer webServer;

inline void resetWifiSettingsAndReboot(bool justReboot = false)
{
  if (!justReboot) { WiFi.disconnect(); delay(500); }
  ESP.restart(); delay(500);
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println(F("\n\rESP07-inAir9 Gateway"));
  Serial.print(F("ChipId=")); Serial.println(ESP.getChipId());

  String defaultSSID = WiFi.SSID();
  Serial.print(F("Saved SSID=")); Serial.println(defaultSSID);

  if (!CheckForUserResetWifiOnBoot()) {
    resetWifiSettingsAndReboot();
  }

  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(true);
  if (!wifiManager.autoConnect(sentrifarm::FALLBACK_SSID.c_str(), sentrifarm::DEFAULT_PASSWORD)) {
    // With the default flow and no timeouts we should never get here
    Serial.println(F("Unable to auto-configure. Reset WIFI DATA. Reboot..."));
    // "forget" the client SSID that we try to connect to on next boot...
    // The delay() calls seem to mititgate otherwise lockup + garbage printed
    resetWifiSettingsAndReboot();
  }

  // We now get to here only when there is a valid client SSID
  // and we have managed to connect to it.
  // The captive portal web server is also now shut down.

  // Note: it would seem that setting an ad-hoc for the client SSD hangs the AP!
  
  // Launch web server.
  webServer.begin();

  // if the user still has it shorted, can be a problem
  // so do do we need an inline resistor? 3.3/330 = 10mA
  if (digitalRead(0) == 0) {
    Serial.println(F("GPIO0 is shorted to ground. Cowardly not connecting output."));
    resetWifiSettingsAndReboot(true);
  }

  //     GPIO12 = MISO (HSPI)
  //     GPIO13 = MOSI (HSPI)
  //     GPIO14 = SCK (HSPI)
  //     GPIO2  = CS
  //     GPIO0  = SXRST

  pinMode(PIN_SX1276_RST,       OUTPUT);
  pinMode(PIN_SX1276_CS,        OUTPUT);
  digitalWrite(PIN_SX1276_MISO, HIGH);
  digitalWrite(PIN_SX1276_MOSI, HIGH);
  digitalWrite(PIN_SX1276_SCK,  HIGH);
  digitalWrite(PIN_SX1276_CS,   HIGH);
  digitalWrite(PIN_SX1276_RST,  HIGH);

  // Reset the sx1276 module
  digitalWrite(PIN_SX1276_RST, LOW);
  delay(10); // spec states to pull low 100us then wait at least 5 ms
  digitalWrite(PIN_SX1276_RST, HIGH);
  delay(50);

  // Dump registers
  Serial.println(F("SX1276 register values"));
  DumpRadioRegisters(&spiSettings);

  SPI.begin();
  radio.Begin();
  radio.SetCarrier(sentrifarm::CARRIER_FREQUENCY_HZ);
  SPI.end();
}

void RadioPollAndDump()
{
  static int radio_calls = 0;
  static int radio_received = 0;
  static int radio_crc = 0;

  bool crc = false;
  byte buf[128] = { 0, 0, 0, 0 };
  byte rxlen = 0;
  radio_calls ++;
  SPI.begin();
  char buf2[384] = { 0 };
  bool ok = radio.ReceiveMessage(buf, sizeof(buf), rxlen, crc);
  radio.Standby();
  SPI.end();
  if (ok || crc) {
    if (crc) { radio_crc ++; }
    else { radio_received ++; }
    // TODO: Flash the LED
    int n = snprintf(buf2, sizeof(buf2), "[RX] %d bytes, crc=%d rssi=%d %02x%02x%02x%02x calls=%d rx=%d crc=%d\n\r",
        rxlen, crc, radio.GetLastRssi(), (int)buf[0], (int)buf[1], (int)buf[2], (int)buf[3], radio_calls, radio_received, radio_crc);
    int i;
    char *p = buf2 + n;
    for (i=0; i < rxlen && i < sizeof(buf2) - 1 - n; i++) { p[i] = (isascii(buf[i]) && !iscntrl(buf[i])) ? buf[i] : '.'; }
    p[i] = 0;
  }
  if (ok || crc) {
    Serial.println(buf2);
  }
}

elapsedMillis loop1;

void loop()
{
  bool radioFault = radio.fault();
  if (loop1 > 10000) { // 10 seconds
    uint32_t carrier_hz = 0;
    if (!radioFault) 
    {
      SPI.begin();
      radio.ReadCarrier(carrier_hz);
      SPI.end();
      webServer.setStatus(String(F("Radio Carrier: ")) + carrier_hz);
    }
    Serial.print(F(" IPAddress=")); Serial.print(WiFi.localIP());
    Serial.print(F(" Carrier="));
    if (radioFault) {
      Serial.print(F("RADIO_NOT_FOUND")); 
    } else {
      Serial.print(carrier_hz); 
    }
    Serial.println();
    loop1 = 0;
  }

  if (radioFault) {
    LEDFlashRadioNotFoundPattern();
    webServer.setStatus(F("Radio not found."));
  } else {
    RadioPollAndDump();
  }
  webServer.handle();
}
