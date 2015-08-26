/*
  Copyright (c) 2015 Andrew McDonnell <bugs@andrewmcdonnell.net>

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

#include "sf-mcu.h"
#include "sf-ioadaptorshield.h"

#include <SPI.h>
#include <Wire.h>

#include <sx1276.h>

namespace Sentrifarm {

  void setup_world(const String& description)
  {
#ifdef ESP8266
    delay(1000); // let last of ESP8266 junk get past
#endif
    Serial.begin(115200);
    Serial.println();
    Serial.print(F("\n\n"));
    Serial.print(F("Sentrifarm : "));
    Serial.print(description);
#if defined(TEENSYDUINO)
    Serial.println(F("TEENSY-LC"));
#elif defined(ESP8266)
    Serial.println(F("ESP8266 ESP-201"));
#endif
  }

  void setup_shield()
  {
    // LED

    pinMode(PIN_LED4,        OUTPUT);

    // i2c

#if defined(TEENSYDUINO)
    // The teensy configuration uses the default pins
#elif defined(ESP8266)
    // Annoyingly this is using deprecated to set the static default because the adafruit library calls Wire.begin()
    Wire.pins(PIN_SDA, PIN_SCL);
#endif

    // SPI

#if defined(TEENSYDUINO)
    SPI.setSCK(PIN_SX1276_SCK);
#endif
    digitalWrite(PIN_SX1276_CS, HIGH);
    digitalWrite(PIN_SX1276_RST, HIGH);
    digitalWrite(PIN_SX1276_MISO, HIGH);
    digitalWrite(PIN_SX1276_MOSI, HIGH);
    digitalWrite(PIN_SX1276_SCK,  HIGH);

    // Radio

    pinMode(PIN_SX1276_RST,  OUTPUT);
    pinMode(PIN_SX1276_CS,   OUTPUT);
  }

  void deep_sleep_and_reset(int ms)
  {
    Serial.print(F("ENTER DEEP SLEEP "));
    Serial.print(ms);
#if defined(ESP8266)
    ESP.deepSleep(ms * 1000, WAKE_RF_DISABLED); // long enough to see current fall on the USB power meter
    delay(500); // Note, deep sleep actually takes a bit to activate, so we want to make sure loop doesnt do anything...
#else
    delay(ms);
    // to simulate ESP8266 behaviour we should reset the CPU here
#endif
  }

  void led4_on()
  {
    digitalWrite(PIN_LED4, LOW);
  }

  void led4_off()
  {
    digitalWrite(PIN_LED4, HIGH);
  }

  // Turn off if already on, delay 0.25s, then twice flash the shield LED for 0.25s. Finishes off.
  void led4_double_short_flash()
  {
    digitalWrite(PIN_LED4, HIGH);
    delay(250);
    digitalWrite(PIN_LED4, LOW);
    delay(250);
    digitalWrite(PIN_LED4, HIGH);
    delay(250);
    digitalWrite(PIN_LED4, LOW);
    delay(250);
    digitalWrite(PIN_LED4, HIGH);
  }

  void reset_radio()
  {
    // Reset the sx1276 module
    digitalWrite(PIN_SX1276_RST, LOW);
    delay(10); // spec states to pull low 100us then wait at least 5 ms
    digitalWrite(PIN_SX1276_RST, HIGH);
    delay(50);
  }

}
