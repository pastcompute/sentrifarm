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

#ifdef TEENSYDUINO
#define Serial Serial1
#endif

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
    Serial.print(" ");
#if defined(TEENSYDUINO)
    Serial.println(F("TEENSY-LC"));
    pinMode(PIN_LED4T,        OUTPUT);
    led4_flash();
#elif defined(ESP8266)
    Serial.println(F("ESP8266 ESP-201"));
#endif
  }

  void setup_shield(bool & beacon_mode, bool & log_mode)
  {
#ifdef DISABLE_LED4
    // DHT11
#else
    // LED
    pinMode(PIN_LED4,        OUTPUT);
#endif

    // Radio
    pinMode(PIN_SX1276_RST,  OUTPUT);
    pinMode(PIN_SX1276_CS,   OUTPUT);

    beacon_mode = false;

#ifdef ESP8266 // why is this not working on the teensy?
    // Before we turn on i2c, see if there is a jumper over SCL
    // in which case go into beacon mode
    pinMode(PIN_SCL, INPUT_PULLUP);
    beacon_mode = digitalRead(PIN_SCL) == 0;

    pinMode(PIN_SDA, INPUT_PULLUP);
    log_mode = digitalRead(PIN_SDA) == 0;
#endif

    // i2c
#if defined(TEENSYDUINO)
    // The teensy configuration uses the default pins
#elif defined(ESP8266)
    // Annoyingly the adafruit library calls Wire.begin()
    // which if we dont set pins, will use the wrong defaults
    // and further, pins() is deprecated and even more annoyingly
    // wire.begin() each time will reset everything...
    if (!beacon_mode && !log_mode) {
      Wire.pins(PIN_SDA, PIN_SCL);
      // Wire.begin doesn t work!
    }
#endif

    // SPI
#if defined(TEENSYDUINO)
    SPI.setSCK(PIN_SX1276_SCK);
#endif
  }

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);

  void deep_sleep_and_reset(int ms)
  {
    Serial.print(F("ENTER DEEP SLEEP "));
    Serial.print(ms);
#if defined(ESP8266)
    ESP.deepSleep(ms * 1000, WAKE_RF_DISABLED); // long enough to see current fall on the USB power meter
    delay(500); // Note, deep sleep actually takes a bit to activate, so we want to make sure loop doesnt do anything...
#elif defined(TEENSYDUINO)
    delay(ms);
    Serial.println(F("Restart"));
    delay(250);
    CPU_RESTART
    delay(250);
    Serial.println(F("Never!"));
#else
    delay(ms);
#endif
  }

  // Hack for teensy LED duplication
#if defined(TEENSYDUINO)
#define WRITE_LED4(pin, x)   digitalWrite(pin, x); digitalWrite(PIN_LED4T, !x);
#else
#define WRITE_LED4(pin, x)   digitalWrite(pin, x);
#endif

#ifdef DISABLE_LED4
  void led4_double_short_flash() {}
  void led4_on() {}
  void led4_off() {}
  void led4_flash() {}
  void led4_short_flash() {}

#else
  void led4_on()
  {
    WRITE_LED4(PIN_LED4, LOW);
  }

  void led4_off()
  {
    WRITE_LED4(PIN_LED4, HIGH);
  }

  void led4_flash()
  {
    WRITE_LED4(PIN_LED4, LOW);
    delay(500);
    WRITE_LED4(PIN_LED4, HIGH);
  }

  void led4_short_flash()
  {
    WRITE_LED4(PIN_LED4, LOW);
    delay(200);
    WRITE_LED4(PIN_LED4, HIGH);
  }

  // Turn off if already on, delay 0.25s, then twice flash the shield LED for 0.25s. Finishes off.
  void led4_double_short_flash()
  {
    WRITE_LED4(PIN_LED4, HIGH);
    delay(250);
    WRITE_LED4(PIN_LED4, LOW);
    delay(250);
    WRITE_LED4(PIN_LED4, HIGH);
    delay(250);
    WRITE_LED4(PIN_LED4, LOW);
    delay(250);
    WRITE_LED4(PIN_LED4, HIGH);
  }
#endif

  void reset_radio()
  {
    digitalWrite(PIN_SX1276_MISO, HIGH);
    digitalWrite(PIN_SX1276_MOSI, HIGH);
    digitalWrite(PIN_SX1276_SCK,  HIGH);
    digitalWrite(PIN_SX1276_CS, HIGH);
    digitalWrite(PIN_SX1276_RST, HIGH);

    // Reset the sx1276 module
    digitalWrite(PIN_SX1276_RST, LOW);
    delay(10); // spec states to pull low 100us then wait at least 5 ms
    digitalWrite(PIN_SX1276_RST, HIGH);
    delay(50);
  }

  void scan_i2c_bus()
  {
    Serial.println("Scanning i2c bus. If this hangs, check the pullup resistors.");
    led4_double_short_flash();
    for (byte addr=3; addr < 127; addr++) {
      char buf[32];
      // snprintf(buf, sizeof(buf), "Scanning i2c addr %02x", (int)addr);      Serial.println(buf);
      Wire.beginTransmission(addr);
      int error = Wire.endTransmission();
      if (error == 0) {
        led4_short_flash();
        snprintf(buf, sizeof(buf), "Found i2c device at %02x", (int)addr);
        Serial.println(buf);
      }
    }
  }

}
