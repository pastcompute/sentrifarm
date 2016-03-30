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
extern "C" {
#include "user_interface.h"
}

#include "mcu.h"
#include <SPI.h>

namespace sentrifarm {

bool CheckForUserResetWifiOnBoot()
{
  // if   the reboot was due to _power_on_ (not reset button)
  // then 
  //      first sleep for 2 seconds 
  //      check if there is a short over GPIO (the pin normally used for programming)
  //      if the short persists over 3 seconds then reset the wifi configuration
  //
  // i.e.
  //      the user needs to power cycle, and then short _after_ 2 seconds...
  //      and continue to hold it for another three seconds
  //      if they are monitoring the serial port the have up to 5 seconds in theory
  //
  // we can help them by turning the serial activity LED onto solid blue
  //
  Serial.print(F("SF: Reset reason: ")); Serial.println(ESP.getResetReason());
  if (ESP.getResetInfoPtr()->reason == REASON_DEFAULT_RST) {
    Serial.print(F("SF: Short GPIO0 now for 5 seconds, to reset WIFI data"));
    SWAP_SERIAL_BEGIN();
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW); // Note, inverted sense
    delay(2000);
    int npress = 0;
    while (digitalRead(0) == 0 && npress < 12) {
      digitalWrite(1, HIGH);
      delay(250);
      digitalWrite(1, LOW);
      delay(250);
      npress ++;
    }
    SWAP_SERIAL_END();
    if (npress >= 12) {
      Serial.println(F("USER RESET of WIFI DATA. Reboot..."));
      // Remain solid blue, to indicate remove the short
      // and wait for the short to be removed
      SWAP_SERIAL_BEGIN();
      pinMode(1, OUTPUT);
      digitalWrite(1, LOW);
      while (digitalRead(0) == 0) {
        delay(250);
      }
      SWAP_SERIAL_END();
      return false;
    }
  }
  return true;
}

void DumpRadioRegisters(SPISettings* spiSettings)
{
  if (!spiSettings) { return; }
  char buf[4];
  SPI.begin();
  for (byte r=0; r <= 0x70; r++) {
    if (r % 8 == 0) {
      snprintf(buf, sizeof(buf), "%02x ", r);
      Serial.print(buf);
    }
    SPI.beginTransaction(*spiSettings);
    digitalWrite(PIN_SX1276_CS, LOW);
    SPI.transfer(r);
    byte value = SPI.transfer(0);
    digitalWrite(PIN_SX1276_CS, HIGH);
    SPI.endTransaction();
    snprintf(buf, sizeof(buf), "%02x ", value);
    Serial.print(buf);
    if (r % 8 == 7) { Serial.println(); }
  }
  SPI.end();
  Serial.println();
}

void LEDFlashRadioNotFoundPattern()
{
  SWAP_SERIAL_BEGIN();
  pinMode(1, OUTPUT);
  // One long, two short
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
  SWAP_SERIAL_END();
}

}
