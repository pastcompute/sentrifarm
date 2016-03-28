#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>    

#include "globals.h"
#include "boot.h"
#include "web.h"
#include "elapsedMillis.h"
#include "sx1276.h"

using sentrifarm::WebServer;
using sentrifarm::CheckForUserResetWifiOnBoot;
using namespace sentrifarm::constants;

#define DEFAULT_PASSWORD "password"

ADC_MODE(ADC_VCC);

#define PIN_SX1276_RST   0
#define PIN_SX1276_CS    2
#define PIN_SX1276_MISO 12
#define PIN_SX1276_MOSI 13
#define PIN_SX1276_SCK  14

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
  if (!wifiManager.autoConnect(FALLBACK_SSID.c_str(), DEFAULT_PASSWORD)) {
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
  //     GPIO15 = CS
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
  char buf[4];
  SPI.begin();
  for (byte r=0; r <= 0x70; r++) {
    if (r % 8 == 0) {
      snprintf(buf, sizeof(buf), "%02x ", r);
      Serial.print(buf);
    }
    SPI.beginTransaction(spiSettings);
    digitalWrite(PIN_SX1276_CS, LOW);
    SPI.transfer(r);
    byte value = SPI.transfer(0);
    digitalWrite(PIN_SX1276_CS, HIGH);
    SPI.endTransaction();
    snprintf(buf, sizeof(buf), "%02x ", value);
    Serial.print(buf);
    if (r % 8 == 7) { Serial.println(); }
  }
  Serial.println();

  delay(100);

  radio.Begin();

  radio.SetCarrier(919000000);
  uint32_t carrier_hz = 0;
  radio.ReadCarrier(carrier_hz);
  Serial.print(F("Carrier: ")); Serial.println(carrier_hz); 

  SPI.end();
}

int radio_calls = 0;
int radio_received = 0;
int radio_crc = 0;

void radio_poll()
{
  if (radio.fault()) {
    webServer.setStatus(F("Radio not found!"));
    Serial.println(F("No radio"));
    yield(); delay(500); Serial.swap();
    pinMode(1, OUTPUT);
    // One long, too short
    digitalWrite(1, LOW);
    delay(1500);
    digitalWrite(1, HIGH);
    delay(300);
    digitalWrite(1, LOW);
    delay(300);
    digitalWrite(1, HIGH);
    delay(300);
    digitalWrite(1, LOW);
    delay(300);
    digitalWrite(1, HIGH);
    Serial.swap(); yield(); delay(500);
    return;
  }

  bool crc = false;
  byte buf[128] = { 0, 0, 0, 0 };
  byte rxlen = 0;
  SPI.begin();
  radio_calls ++;
  char buf2[384] = { 0 };
  bool ok = radio.ReceiveMessage(buf, sizeof(buf), rxlen, crc);
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
  radio.Standby();
  SPI.end();
  if (ok || crc) {
    Serial.println(buf2);
  }
}

elapsedMillis loop1;

void loop()
{
  if (loop1 > 10000) {
    loop1 = 0;
    Serial.print("IP address:"); Serial.println(WiFi.localIP());
  }
  webServer.handle();
  radio_poll();
}
